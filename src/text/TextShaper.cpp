#include "text/TextShaper.h"
#include "core/Application.h"
#include "core/Logger.h"
#include "hb-ot.h"
#include "style/StyleValue.h"
#include "text/FontCoverage.h"
#include "text/FontRegistry.h"
#include "text/FontTypes.h"
#include "utils/ScopedProfiler.h"
#include <algorithm>
#include <functional>
#include <optional>
#include <vector>

namespace mocca::mtext::detail
{
	auto buildParagraphs(std::string_view content)
		-> std::vector<ShapedParagraph>
	{
		std::vector<ShapedParagraph> paragraphs;
		size_t start = 0;
		for (size_t i = 0; i <= content.size(); i++)
		{
			if (i == content.size() || content[i] == '\n')
			{
				paragraphs.push_back(
					ShapedParagraph{
						.Start = static_cast<uint32_t>(start),
						.End = static_cast<uint32_t>(i),
						.BaseLevel = 0,
					}
				);
				start = i + 1;
			}
		}
		return paragraphs;
	}

	auto hashVariations(const std::vector<hb_variation_t>& variations) -> size_t
	{
		size_t hash = std::hash<size_t>{}(variations.size());
		for (const auto& variation : variations)
		{
			mocca::hashCombine(hash, std::hash<uint32_t>{}(variation.tag));
			mocca::hashCombine(hash, std::hash<float>{}(variation.value));
		}
		return hash;
	}

	struct FontMetrics
	{
		int32_t Ascender = 0;
		int32_t Descender = 0;
		int32_t LineGap = 0;
	};

	auto metricsFor(hb_font_t* font, uint32_t upem) -> FontMetrics
	{
		hb_font_extents_t extents{};
		if (hb_font_get_h_extents(font, &extents) != 0)
		{
			return FontMetrics{
				.Ascender = extents.ascender,
				.Descender = -extents.descender,
				.LineGap = extents.line_gap,
			};
		}

		return FontMetrics{
			.Ascender = static_cast<int32_t>(static_cast<float>(upem) * 0.8F),
			.Descender = static_cast<int32_t>(static_cast<float>(upem) * 0.2F),
			.LineGap = 0,
		};
	}

	auto shapeItem(
		hb_font_t* font,
		hb_face_t* face,
		std::string_view content,
		const TextItem& item,
		const std::vector<hb_feature_t>& features
	) -> ShapedItem
	{
		const char* data = content.empty() ? "" : content.data();
		auto offset = static_cast<int>(item.Start);
		auto length = static_cast<int>(item.End - item.Start);

		hb_buffer_t* buffer = hb_buffer_create();
		hb_buffer_add_utf8(
			buffer,
			data,
			static_cast<int>(content.size()),
			offset,
			length
		);
		hb_buffer_set_direction(buffer, HB_DIRECTION_LTR);
		hb_buffer_set_script(buffer, static_cast<hb_script_t>(item.Script));
		hb_buffer_guess_segment_properties(buffer);
		hb_shape(
			font,
			buffer,
			features.data(),
			static_cast<unsigned>(features.size())
		);

		unsigned count = 0;
		const hb_glyph_info_t* infos =
			hb_buffer_get_glyph_infos(buffer, &count);
		const hb_glyph_position_t* positions =
			hb_buffer_get_glyph_positions(buffer, &count);

		uint32_t upem = hb_face_get_upem(face);

		ShapedItem out;
		out.Face = item.Face;
		out.Upem = upem;
		out.Start = item.Start;
		out.End = item.End;
		out.Level = 0;
		out.Glyphs.reserve(count);

		for (unsigned i = 0; i < count; i++)
		{
			out.Glyphs.push_back(
				ShapeGlyph{
					.GlyphId = infos[i].codepoint,
					.XAdvance = positions[i].x_advance,
					.YAdvance = positions[i].y_advance,
					.XOffset = positions[i].x_offset,
					.YOffset = positions[i].y_offset,
					.Cluster = infos[i].cluster,
				}
			);
		}

		FontMetrics metrics = metricsFor(font, upem);
		out.Ascender = metrics.Ascender;
		out.Descender = metrics.Descender;
		out.LineGap = metrics.LineGap;

		hb_buffer_destroy(buffer);
		return out;
	}

	struct AxisRange
	{
		float Min = 0;
		float Max = 0;
	};

	auto findAxis(hb_face_t* face, hb_tag_t tag) -> std::optional<AxisRange>;

	auto buildVariations(
		hb_face_t* face,
		const FontRequest& request,
		uint32_t handle
	) -> std::vector<hb_variation_t>;

	auto buildFeatures(uint32_t handle) -> std::vector<hb_feature_t>;

	auto TextShaper::FontFor(
		hb_face_t* face,
		FontFaceId id,
		const std::vector<hb_variation_t>& variations
	) -> hb_font_t*
	{
		FontKey key{.Face = id, .Variations = hashVariations(variations)};
		if (auto it = fonts.find(key); it != fonts.end())
		{
			return it->second.get();
		}

		unsigned upem = hb_face_get_upem(face);
		hb_font_t* font = hb_font_create(face);
		hb_font_set_scale(font, static_cast<int>(upem), static_cast<int>(upem));
		hb_font_set_variations(
			font,
			variations.data(),
			static_cast<unsigned>(variations.size())
		);
		hb_ot_font_set_funcs(font);

		fonts.emplace(key, FontPtr{font, hb_font_destroy});
		return font;
	}

	auto TextShaper::ReportNoFace() -> std::shared_ptr<const ShapedText>
	{
		if (!warnedNoFace)
		{
			warnedNoFace = true;
			mc_error(
				ErrorCode::InvalidArgument,
				"no font face resolved for the requested family stack"
			);
		}
		return nullptr;
	}

	auto TextShaper::Shape(
		std::string_view content,
		const TextLayoutStyle& style
	) -> std::shared_ptr<const ShapedText>
	{
		mc_profileFunction();

		auto& registry = Application::main->Fonts();
		auto stack = registry.MatchStack(style.Families, style.Request);

		std::vector<FontFaceId> faces;
		faces.reserve(stack.Entries.size());
		for (const auto& entry : stack.Entries)
		{
			if (entry.Match)
			{
				faces.push_back(entry.Match.Id);
			}
		}

		if (faces.empty())
		{
			return ReportNoFace();
		}

		hb_face_t* strut = registry.Face(faces.front());
		if (strut == nullptr)
		{
			return ReportNoFace();
		}

		auto strutVariations =
			buildVariations(strut, style.Request, style.Variations);

		ShapeKey key{
			.Content = std::string(content),
			.Faces = faces,
			.Variations = hashVariations(strutVariations),
			.Features = style.Features,
		};

		if (auto hit = cache.Find(key))
		{
			stats.ShapeHits++;
			return hit;
		}

		stats.ShapeCalls++;

		auto features = buildFeatures(style.Features);

		uint32_t strutUpem = hb_face_get_upem(strut);
		FontMetrics strutMetrics = metricsFor(
			FontFor(strut, faces.front(), strutVariations),
			strutUpem
		);

		auto shaped = std::make_shared<ShapedText>();
		shaped->Id = nextShapedId++;
		shaped->Variations = style.Variations;
		shaped->Features = style.Features;
		shaped->Content = std::string(content);

		shaped->StrutFace = faces.front();
		shaped->StrutUpem = strutUpem;
		shaped->StrutAscender = strutMetrics.Ascender;
		shaped->StrutDescender = strutMetrics.Descender;
		shaped->StrutLineGap = strutMetrics.LineGap;

		shaped->Paragraphs = buildParagraphs(content);

		for (const TextItem& item : TextItem::Itemize(content, faces))
		{
			hb_face_t* face = registry.Face(item.Face);
			if (face == nullptr)
			{
				continue;
			}

			auto variations =
				buildVariations(face, style.Request, style.Variations);
			hb_font_t* font = FontFor(face, item.Face, variations);

			shaped->Items.push_back(
				shapeItem(font, face, content, item, features)
			);
		}

		cache.Insert(key, shaped);
		return shaped;
	}

	auto TextShaper::GetStats() -> TextShaperStats
	{
		return stats;
	}

	void TextShaper::PurgeCache()
	{
		cache.Clear();
		fonts.clear();
		FontCoverage::Clear();
		warnedNoFace = false;
	}

	auto findAxis(hb_face_t* face, hb_tag_t tag) -> std::optional<AxisRange>
	{
		if (hb_ot_var_has_data(face) == 0)
		{
			return std::nullopt;
		}

		hb_ot_var_axis_info_t axes[32];
		unsigned count = 32;
		hb_ot_var_get_axis_infos(face, 0, &count, axes);
		for (unsigned i = 0; i < count; i++)
		{
			if (axes[i].tag == tag)
			{
				return AxisRange{
					.Min = axes[i].min_value,
					.Max = axes[i].max_value,
				};
			}
		}
		return std::nullopt;
	}

	auto buildVariations(
		hb_face_t* face,
		const FontRequest& request,
		uint32_t handle
	) -> std::vector<hb_variation_t>
	{
		std::vector<hb_variation_t> out;

		auto set = [&](hb_tag_t tag, float value) -> void
		{
			auto axis = findAxis(face, tag);
			if (!axis)
			{
				return;
			}

			float clamped = std::clamp(value, axis->Min, axis->Max);
			for (auto& existing : out)
			{
				if (existing.tag == tag)
				{
					existing.value = clamped;
					return;
				}
			}
			out.push_back(hb_variation_t{tag, clamped});
		};

		set(HB_TAG('w', 'g', 'h', 't'), request.Weight);
		set(HB_TAG('w', 'd', 't', 'h'), request.Stretch);
		if (request.Style == FontStyleEnum::Oblique)
		{
			set(HB_TAG('s', 'l', 'n', 't'), -request.Slant);
		}

		if (handle != 0)
		{
			for (const auto& axis :
				 styles::detail::resolveFontVariations(handle).Axes)
			{
				set(axis.Tag, axis.Value);
			}
		}
		return out;
	}

	auto buildFeatures(uint32_t handle) -> std::vector<hb_feature_t>
	{
		std::vector<hb_feature_t> out;
		if (handle == 0)
		{
			return out;
		}

		const auto& features = styles::detail::resolveFontFeatures(handle);
		out.reserve(features.Axes.size());
		for (const auto& feature : features.Axes)
		{
			out.push_back(
				hb_feature_t{
					.tag = feature.Tag,
					.value = feature.Value,
					.start = 0,
					.end = HB_FEATURE_GLOBAL_END,
				}
			);
		}
		return out;
	}
}

namespace mocca::mtext
{
	auto TextLine::XOffset(float boxWidth, TextAlignment align) const -> float
	{
		switch (align)
		{
		case TextAlignment::End:
		case TextAlignment::Right:
			return boxWidth - Width;
		case TextAlignment::Center:
			return (boxWidth - Width) * 0.5F;
		case TextAlignment::Start:
		case TextAlignment::Left:
		case TextAlignment::Justify:
		case TextAlignment::MatchParent:
		default:
			return 0;
		}
	}
}