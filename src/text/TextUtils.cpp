#include "text/TextUtils.h"
#include "text/TextLayout.h"
#include "linebreak.h"
#include <algorithm>
#include <utility>

namespace mocca::mtext::detail
{
	auto scaleFor(const ShapedItem& item, float sizePx) -> float
	{
		return (item.Upem == 0) ? 0.0F : sizePx / static_cast<float>(item.Upem);
	}

	auto baseLevelFor(
		const std::vector<ShapedParagraph>& paragraphs,
		uint32_t offset
	) -> uint8_t
	{
		uint8_t level = 0;
		for (const ShapedParagraph& paragraph : paragraphs)
		{
			if (paragraph.Start > offset)
			{
				break;
			}
			level = paragraph.BaseLevel;
		}
		return level;
	}

	auto reorderRuns(std::vector<ShapedRun>& runs) -> void
	{
		if (runs.size() < 2)
		{
			return;
		}

		uint8_t highest = 0;
		uint8_t lowestOdd = 0xFF;
		for (const ShapedRun& run : runs)
		{
			highest = std::max(highest, run.Level);
			if ((run.Level & 1) != 0)
			{
				lowestOdd = std::min(lowestOdd, run.Level);
			}
		}

		for (int level = highest; level >= static_cast<int>(lowestOdd); level--)
		{
			for (size_t i = 0; i < runs.size();)
			{
				if (std::cmp_less(runs[i].Level, level))
				{
					i++;
					continue;
				}

				size_t j = i;
				while (j < runs.size() && runs[j].Level >= level)
				{
					j++;
				}
				std::reverse(runs.begin() + i, runs.begin() + j);
				i = j;
			}
		}
	}

	auto buildClusters(const ShapedText& shaped, const TextLayoutStyle& style)
		-> std::vector<Cluster>
	{
		const std::string& content = shaped.Content;
		std::vector<Cluster> clusters;

		for (uint32_t itemIndex = 0; itemIndex < shaped.Items.size();
			 itemIndex++)
		{
			const ShapedItem& item = shaped.Items[itemIndex];
			float scale = scaleFor(item, style.SizePx);

			std::vector<Cluster> groups;
			groups.reserve(item.Glyphs.size());
			for (size_t g = 0; g < item.Glyphs.size();)
			{
				uint32_t byte = item.Glyphs[g].Cluster;
				float width = 0;
				size_t j = g;
				while (j < item.Glyphs.size() && item.Glyphs[j].Cluster == byte)
				{
					width += static_cast<float>(item.Glyphs[j].XAdvance) *
							 scale;
					j++;
				}

				bool space = byte < content.size() &&
							 isSpaceByte(content[byte]);
				bool hanging = byte < content.size() &&
							   isHangingByte(content[byte]);
				width += style.LetterSpacingPx +
						 (space ? style.WordSpacingPx : 0.0F);

				groups.push_back(
					Cluster{
						.Start = byte,
						.Item = itemIndex,
						.GlyphBegin = static_cast<uint32_t>(g),
						.GlyphEnd = static_cast<uint32_t>(j),
						.Advance = width,
						.IsSpace = space,
						.Hanging = hanging,
					}
				);
				g = j;
			}

			bool rtl = (item.Level & 1) != 0;
			size_t count = groups.size();
			for (size_t k = 0; k < count; k++)
			{
				size_t logical = rtl ? count - 1 - k : k;
				Cluster cluster = groups[logical];
				cluster.End = (k + 1 < count)
								  ? groups[rtl ? count - 2 - k : k + 1].Start
								  : item.End;
				clusters.push_back(cluster);
			}
		}

		return clusters;
	}

	auto LineEmitter::Emit(size_t begin, size_t end) -> float
	{
		size_t n = Clusters.size();
		size_t contentSize = Shaped.Content.size();

		size_t glyphBound = end;
		while (glyphBound > begin && Clusters[glyphBound - 1].Hanging)
		{
			glyphBound--;
		}

		TextLine line;
		line.Start = (begin < n) ? Clusters[begin].Start
								 : static_cast<uint32_t>(contentSize);
		line.End = (end < n) ? Clusters[end].Start
							 : static_cast<uint32_t>(contentSize);
		line.BaseLevel = baseLevelFor(Shaped.Paragraphs, line.Start);

		float width = 0;
		for (size_t k = begin; k < glyphBound; k++)
		{
			width += Clusters[k].Advance;
		}
		line.Width = width;

		auto lineIndex = static_cast<float>(Layout.Lines.size());
		line.Baseline = (lineIndex * LineHeight) + HalfLeading + Ascent;

		std::vector<ShapedRun> runs;
		for (size_t k = begin; k < glyphBound;)
		{
			uint32_t itemIndex = Clusters[k].Item;
			const ShapedItem& item = Shaped.Items[itemIndex];
			float scale = scaleFor(item, Style.SizePx);

			size_t j = k;
			uint32_t glyphBegin = Clusters[k].GlyphBegin;
			uint32_t glyphEnd = Clusters[k].GlyphEnd;
			while (j < glyphBound && Clusters[j].Item == itemIndex)
			{
				glyphBegin = std::min(glyphBegin, Clusters[j].GlyphBegin);
				glyphEnd = std::max(glyphEnd, Clusters[j].GlyphEnd);
				j++;
			}

			std::vector<float> spacing(glyphEnd - glyphBegin, 0.0F);
			for (size_t c = k; c < j; c++)
			{
				const Cluster& cluster = Clusters[c];
				spacing[cluster.GlyphEnd - 1 - glyphBegin] +=
					Style.LetterSpacingPx +
					(cluster.IsSpace ? Style.WordSpacingPx : 0.0F);
			}

			ShapedRun run;
			run.Face = item.Face;
			run.Size = Style.SizePx;
			run.Start = Clusters[k].Start;
			run.End = Clusters[j - 1].End;
			run.Level = item.Level;
			run.Glyphs.reserve(glyphEnd - glyphBegin);

			float pen = 0;
			for (uint32_t g = glyphBegin; g < glyphEnd; g++)
			{
				const ShapeGlyph& src = item.Glyphs[g];
				float advance = static_cast<float>(src.XAdvance) * scale;
				run.Glyphs.push_back(
					ShapedGlyph{
						.GlyphId = src.GlyphId,
						.X = pen,
						.XAdvance = advance,
						.XOffset = static_cast<float>(src.XOffset) * scale,
						.YOffset = static_cast<float>(src.YOffset) * scale,
						.Cluster = src.Cluster,
					}
				);
				pen += advance + spacing[g - glyphBegin];
			}
			run.Width = pen;

			runs.push_back(std::move(run));
			k = j;
		}

		reorderRuns(runs);

		line.RunBegin = static_cast<uint32_t>(Layout.Runs.size());
		float penX = 0;
		for (ShapedRun& run : runs)
		{
			run.X = penX;
			penX += run.Width;
			Layout.Runs.push_back(std::move(run));
		}
		line.RunEnd = static_cast<uint32_t>(Layout.Runs.size());

		line.ClusterBegin = static_cast<uint32_t>(Layout.Clusters.size());
		for (uint32_t r = line.RunBegin; r < line.RunEnd; r++)
		{
			const ShapedRun& run = Layout.Runs[r];
			for (size_t g = 0; g < run.Glyphs.size();)
			{
				uint32_t byte = run.Glyphs[g].Cluster;
				size_t h = g;
				while (h < run.Glyphs.size() && run.Glyphs[h].Cluster == byte)
				{
					h++;
				}

				float x0 = run.X + run.Glyphs[g].X;
				float x1 = (h < run.Glyphs.size())
							   ? run.X + run.Glyphs[h].X
							   : run.X + run.Width;

				auto it = std::lower_bound(
					Clusters.begin(),
					Clusters.end(),
					byte,
					[](const Cluster& c, uint32_t b) -> bool
					{ return c.Start < b; }
				);
				uint32_t end =
					(it != Clusters.end() && it->Start == byte) ? it->End : byte;

				Layout.Clusters.push_back(
					TextClusterBox{
						.Start = byte,
						.End = end,
						.X = x0,
						.Width = x1 - x0,
						.Level = run.Level,
					}
				);
				g = h;
			}
		}
		line.ClusterEnd = static_cast<uint32_t>(Layout.Clusters.size());

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