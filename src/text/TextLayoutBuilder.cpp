#include "text/TextLayoutBuilder.h"
#include "TextResolve.h"
#include "text/LayoutCache.h"
#include "text/TextShaper.h"
#include "text/TextUtils.h"
#include <algorithm>
#include <bit>
#include <cmath>

namespace mocca::mtext::detail
{
	constexpr float FitEpsilon = 1.0e-3F;
	constexpr size_t NoBreak = SIZE_MAX;

	auto TextLayoutBuilder::BuildLayout(
		const ShapedText& shaped,
		const TextLayoutStyle& style,
		float maxWidth,
		bool wrap
	) -> std::shared_ptr<const TextLayout>
	{
		auto layout = std::make_shared<TextLayout>();
		layout->Face = shaped.Face;
		layout->FontSize = style.SizePx;

		float scale = static_cast<float>(style.SizePx) /
					  static_cast<float>(shaped.Upem);
		float ascent = static_cast<float>(shaped.Ascender) * scale;
		float descent = static_cast<float>(shaped.Descender) * scale;
		float lineGap = static_cast<float>(shaped.LineGap) * scale;

		float lineHeight = ascent + descent + lineGap;
		switch (style.LineHeight)
		{
		case LineHeightKind::Number:
			lineHeight = style.LineHeightValue * style.SizePx;
			break;
		case LineHeightKind::Px:
			lineHeight = style.LineHeightValue;
			break;
		case LineHeightKind::Normal:
			break;
		}

		float halfLeading = (lineHeight - (ascent + descent)) * 0.5F;

		layout->Ascent = ascent;
		layout->Descent = descent;
		layout->LineHeight = lineHeight;

		if (shaped.Content.empty())
		{
			return layout;
		}

		std::vector<Cluster> clusters = buildClusters(shaped, style, scale);
		size_t n = clusters.size();
		std::vector<BreakFlag> breaks = breakOpportunities(shaped.Content);

		auto opportunity = [&](size_t i) -> BreakFlag
		{
			size_t byteOffset = (i < n) ? clusters[i].Byte
										: shaped.Content.size();
			return breaks[byteOffset];
		};

		detail::LineEmitter emitter{
			.Layout = *layout,
			.Shaped = shaped,
			.Style = style,
			.Clusters = clusters,
			.Scale = scale,
			.LineHeight = lineHeight,
			.HalfLeading = halfLeading,
			.Ascent = ascent,
		};

		float maxLineWidth = 0;
		float raw = 0;
		float trim = 0;
		size_t lineBegin = 0;
		size_t i = 0;
		size_t lastAllow = NoBreak;

		for (;;)
		{
			BreakFlag opp = opportunity(i);

			if (opp == BreakFlag::Must && i > lineBegin)
			{
				maxLineWidth =
					std::max(maxLineWidth, emitter.Emit(lineBegin, i - 1));
				lineBegin = i;
				if (i >= n)
				{
					maxLineWidth = std::max(maxLineWidth, emitter.Emit(n, n));
					break;
				}
				raw = 0;
				trim = 0;
				lastAllow = NoBreak;
				continue;
			}

			if (i >= n)
			{
				maxLineWidth =
					std::max(maxLineWidth, emitter.Emit(lineBegin, n));
				break;
			}

			if (opp == BreakFlag::Allow && i > lineBegin)
			{
				lastAllow = i;
			}

			float newRaw = raw + clusters[i].Width;
			float newTrim = clusters[i].Hanging ? trim : newRaw;

			if (wrap && i > lineBegin && newTrim > maxWidth + FitEpsilon)
			{
				size_t at = (lastAllow != NoBreak && lastAllow > lineBegin)
								? lastAllow
								: i;
				maxLineWidth =
					std::max(maxLineWidth, emitter.Emit(lineBegin, at));
				lineBegin = at;
				i = at;
				raw = 0;
				trim = 0;
				lastAllow = NoBreak;
				continue;
			}

			raw = newRaw;
			trim = newTrim;
			i++;
		}

		layout->Size = Vector2{
			.X = maxLineWidth,
			.Y = static_cast<float>(layout->Lines.size()) * lineHeight,
		};
		return layout;
	}

	auto TextLayoutBuilder::LayoutFor(
		const std::shared_ptr<const ShapedText>& shaped,
		const TextLayoutStyle& style,
		float maxWidth,
		bool wrap
	) -> std::shared_ptr<const TextLayout>
	{
		LayoutKey key;
		key.ShapedId = shaped->Id;
		key.SizeBits = std::bit_cast<uint32_t>(style.SizePx);
		key.LineHeightKind = static_cast<uint8_t>(style.LineHeight);
		key.LineHeightBits = std::bit_cast<uint32_t>(style.LineHeightValue);
		key.LetterSpacingBits = std::bit_cast<uint32_t>(style.LetterSpacingPx);
		key.WordSpacingBits = std::bit_cast<uint32_t>(style.WordSpacingPx);
		key.MaxWidthBits = std::bit_cast<uint32_t>(maxWidth);
		key.Wrap = wrap;

		if (auto hit = cache.Find(key))
		{
			stats.LayoutHits++;
			return hit;
		}

		auto layout = BuildLayout(*shaped, style, maxWidth, wrap);
		stats.LayoutBuilds++;
		cache.Insert(key, layout);
		return layout;
	}

	void TextLayoutBuilder::PurgeCache()
	{
		cache.Clear();
		stats = LayoutStats{};
	}

	auto TextLayoutBuilder::GetStats() -> LayoutStats
	{
		return stats;
	}

	auto TextLayoutBuilder::ResolveTextLayout(
		const styles::ComputedStyle& style,
		std::string_view content,
		float maxWidth,
		bool wrap
	) -> std::shared_ptr<const TextLayout>
	{
		float snapped = std::floor(maxWidth);
		TextLayoutStyle layoutStyle = detail::ResolveTextLayoutStyle(style);

		auto shaped = detail::TextShaper::Shape(content, layoutStyle);
		if (!shaped)
		{
			auto empty = std::make_shared<TextLayout>();
			empty->Face = SIZE_MAX;
			empty->FontSize = layoutStyle.SizePx;
			return empty;
		}

		return LayoutFor(shaped, layoutStyle, snapped, wrap);
	}
}