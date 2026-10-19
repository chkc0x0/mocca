#include "text/TextUtils.h"
#include "text/TextLayout.h"
#include "linebreak.h"

namespace mocca::mtext::detail
{
	auto buildClusters(
		const ShapedText& shaped,
		const TextLayoutStyle& style,
		float scale
	) -> std::vector<Cluster>
	{
		const std::string& content = shaped.Content;
		std::vector<Cluster> clusters;
		clusters.reserve(shaped.Glyphs.size());

		for (size_t g = 0; g < shaped.Glyphs.size();)
		{
			uint32_t byte = shaped.Glyphs[g].Cluster;
			float width = 0;
			size_t j = g;
			while (j < shaped.Glyphs.size() && shaped.Glyphs[j].Cluster == byte)
			{
				width += static_cast<float>(shaped.Glyphs[j].XAdvance) * scale;
				j++;
			}

			bool space = byte < content.size() && isSpaceByte(content[byte]);
			bool hanging = byte < content.size() &&
						   isHangingByte(content[byte]);
			width += style.LetterSpacingPx +
					 (space ? style.WordSpacingPx : 0.0F);

			clusters.push_back(
				Cluster{
					.StartGlyph = static_cast<uint32_t>(g),
					.EndGlyph = static_cast<uint32_t>(j),
					.Byte = byte,
					.Width = width,
					.Space = space,
					.Hanging = hanging,
				}
			);
			g = j;
		}

		return clusters;
	}

	auto LineEmitter::Emit(size_t begin, size_t end) -> float
	{
		size_t n = Clusters.size();
		size_t contentSize = Shaped.Content.size();

		TextLine line;
		line.Start = (begin < n) ? Clusters[begin].Byte
								 : static_cast<uint32_t>(contentSize);
		line.End = (end < n) ? Clusters[end].Byte
							 : static_cast<uint32_t>(contentSize);

		float width = 0;
		for (size_t k = begin; k < end; k++)
		{
			width += Clusters[k].Width;
		}
		for (size_t k = end; k > begin && Clusters[k - 1].Hanging; k--)
		{
			width -= Clusters[k - 1].Width;
		}
		line.Width = width;

		auto lineIndex = static_cast<float>(Layout.Lines.size());
		line.Baseline = (lineIndex * LineHeight) + HalfLeading + Ascent;

		ShapedRun run;
		run.Face = Shaped.Face;
		run.Size = Style.SizePx;
		float penX = 0;
		for (size_t k = begin; k < end; k++)
		{
			const Cluster& cluster = Clusters[k];
			for (uint32_t g = cluster.StartGlyph; g < cluster.EndGlyph; g++)
			{
				const ShapeGlyph& src = Shaped.Glyphs[g];
				float advance = static_cast<float>(src.XAdvance) * Scale;
				run.Glyphs.push_back(
					ShapedGlyph{
						.GlyphId = src.GlyphId,
						.X = penX,
						.XAdvance = advance,
						.XOffset = static_cast<float>(src.XOffset) * Scale,
						.YOffset = static_cast<float>(src.YOffset) * Scale,
						.Cluster = src.Cluster,
					}
				);
				penX += advance;
			}
			penX += Style.LetterSpacingPx +
					(cluster.Space ? Style.WordSpacingPx : 0.0F);
		}
		run.Width = penX;

		line.RunBegin = static_cast<uint32_t>(Layout.Runs.size());
		line.RunEnd = line.RunBegin + 1;

		Layout.Runs.push_back(std::move(run));
		Layout.Lines.push_back(line);
		return line.Width;
	}

	auto breakOpportunities(std::string_view utf8) -> std::vector<BreakFlag>
	{
		std::vector<BreakFlag> out(utf8.size() + 1, BreakFlag::No);
		if (utf8.empty())
		{
			return out;
		}

		static const bool Initialized = [] -> bool
		{
			init_linebreak();
			return true;
		}();
		(void)Initialized;

		std::vector<char> flags(utf8.size(), LINEBREAK_NOBREAK);
		set_linebreaks_utf8(
			reinterpret_cast<const utf8_t*>(utf8.data()),
			utf8.size(),
			"en",
			flags.data()
		);

		for (size_t i = 0; i < utf8.size(); i++)
		{
			// libunibreak reports after byte i
			switch (flags[i])
			{
			case LINEBREAK_MUSTBREAK:
				out[i + 1] = BreakFlag::Must;
				break;
			case LINEBREAK_ALLOWBREAK:
				out[i + 1] = BreakFlag::Allow;
				break;
			default:
				break;
			}
		}

		return out;
	}
}