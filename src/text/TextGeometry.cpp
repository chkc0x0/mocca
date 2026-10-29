#include "text/TextGeometry.h"
#include <algorithm>
#include <cmath>

namespace mocca::mtext
{
	auto lineOffsetX(
		const TextLayout& layout,
		uint32_t lineIndex,
		float boxWidth,
		styles::TextAlignment align
	) -> float
	{
		return layout.Lines[lineIndex].XOffset(boxWidth, align);
	}

	auto caretXInLine(
		const TextLayout& layout,
		uint32_t lineIndex,
		uint32_t offset,
		float boxWidth,
		styles::TextAlignment align
	) -> float
	{
		const TextLine& line = layout.Lines[lineIndex];
		float x0 = line.XOffset(boxWidth, align);
		bool rtlLine = (line.BaseLevel & 1) != 0;

		for (uint32_t c = line.ClusterBegin; c < line.ClusterEnd; c++)
		{
			const TextClusterBox& box = layout.Clusters[c];
			if (box.Start == offset)
			{
				return x0 + (box.IsRtl() ? box.X + box.Width : box.X);
			}
			if (box.End == offset)
			{
				return x0 + (box.IsRtl() ? box.X : box.X + box.Width);
			}
		}

		if (offset <= line.Start)
		{
			return x0 + (rtlLine ? line.Width : 0.0F);
		}

		return x0 + (rtlLine ? 0.0F : line.Width);
	}

	auto TextGeometry::LineAtPoint(const TextLayout& layout, float y) -> uint32_t
	{
		if (layout.Lines.empty() || layout.LineHeight <= 0.0F)
		{
			return 0;
		}

		auto index = static_cast<int64_t>(std::floor(y / layout.LineHeight));
		auto last = static_cast<int64_t>(layout.Lines.size() - 1);
		index = std::clamp<int64_t>(index, 0, last);
		return static_cast<uint32_t>(index);
	}

	auto TextGeometry::LineOf(const TextLayout& layout, TextPosition position)
		-> uint32_t
	{
		size_t count = layout.Lines.size();
		if (count == 0)
		{
			return 0;
		}

		for (size_t i = 0; i < count; i++)
		{
			const TextLine& line = layout.Lines[i];
			if (position.Offset < line.End)
			{
				return static_cast<uint32_t>(i);
			}
			if (position.Offset == line.End)
			{
				bool softWrap = (i + 1 < count) &&
								layout.Lines[i + 1].Start == line.End;
				if (softWrap && position.Affinity == CaretAffinity::Downstream)
				{
					return static_cast<uint32_t>(i + 1);
				}
				return static_cast<uint32_t>(i);
			}
		}

		return static_cast<uint32_t>(count - 1);
	}

	auto TextGeometry::LineStart(
		const TextLayout& layout,
		TextPosition position
	) -> TextPosition
	{
		if (layout.Lines.empty())
		{
			return TextPosition{};
		}

		uint32_t line = LineOf(layout, position);
		return TextPosition{
			.Offset=layout.Lines[line].Start,
			.Affinity=CaretAffinity::Downstream,
		};
	}

	auto TextGeometry::LineEnd(const TextLayout& layout, TextPosition position)
		-> TextPosition
	{
		if (layout.Lines.empty())
		{
			return TextPosition{};
		}

		uint32_t line = LineOf(layout, position);
		return TextPosition{.Offset=layout.Lines[line].End, .Affinity=CaretAffinity::Upstream};
	}

	auto TextGeometry::PositionAtPoint(
		const TextLayout& layout,
		Vector2 point,
		float boxWidth,
		styles::TextAlignment align
	) -> TextPosition
	{
		if (layout.Lines.empty())
		{
			return TextPosition{};
		}

		uint32_t lineIndex = LineAtPoint(layout, point.Y);
		const TextLine& line = layout.Lines[lineIndex];
		if (line.ClusterBegin >= line.ClusterEnd)
		{
			return TextPosition{.Offset=line.Start, .Affinity=CaretAffinity::Downstream};
		}

		float localX = point.X -
					   lineOffsetX(layout, lineIndex, boxWidth, align);

		const TextClusterBox& first = layout.Clusters[line.ClusterBegin];
		const TextClusterBox& last = layout.Clusters[line.ClusterEnd - 1];

		auto withAffinity = [&](uint32_t offset) -> TextPosition
		{
			return TextPosition{
				.Offset = offset,
				.Affinity = offset == line.End ? CaretAffinity::Upstream
											   : CaretAffinity::Downstream,
			};
		};

		if (localX <= first.X)
		{
			return withAffinity(first.IsRtl() ? first.End : first.Start);
		}
		if (localX >= last.X + last.Width)
		{
			bool rtlLine = (line.BaseLevel & 1) != 0;
			return withAffinity(rtlLine ? line.Start : line.End);
		}

		for (uint32_t c = line.ClusterBegin; c < line.ClusterEnd; c++)
		{
			const TextClusterBox& box = layout.Clusters[c];
			if (localX >= box.X + box.Width)
			{
				continue;
			}

			float mid = box.X + (box.Width * 0.5F);
			uint32_t offset = box.IsRtl()
								  ? (localX < mid ? box.End : box.Start)
								  : (localX < mid ? box.Start : box.End);
			return withAffinity(offset);
		}

		return TextPosition{.Offset=line.End, .Affinity=CaretAffinity::Upstream};
	}

	auto TextGeometry::CaretAt(
		const TextLayout& layout,
		TextPosition position,
		float boxWidth,
		styles::TextAlignment align
	) -> TextCaret
	{
		if (layout.Lines.empty())
		{
			return TextCaret{};
		}

		uint32_t lineIndex = LineOf(layout, position);
		return TextCaret{
			.Line = lineIndex,
			.X = caretXInLine(
				layout,
				lineIndex,
				position.Offset,
				boxWidth,
				align
			),
			.Y = static_cast<float>(lineIndex) * layout.LineHeight,
			.Height = layout.LineHeight,
		};
	}

	auto TextGeometry::SelectionRects(
		const TextLayout& layout,
		uint32_t anchor,
		uint32_t active,
		float boxWidth,
		styles::TextAlignment align
	) -> std::vector<Rectangle>
	{
		std::vector<Rectangle> rects;
		if (anchor == active || layout.Lines.empty())
		{
			return rects;
		}

		uint32_t lo = std::min(anchor, active);
		uint32_t hi = std::max(anchor, active);

		for (size_t i = 0; i < layout.Lines.size(); i++)
		{
			const TextLine& line = layout.Lines[i];
			if (hi <= line.Start || lo >= line.End)
			{
				continue;
			}

			uint32_t start = std::max(lo, line.Start);
			uint32_t end = std::min(hi, line.End);
			if (start >= end)
			{
				continue;
			}

			auto index = static_cast<uint32_t>(i);
			float x1 = caretXInLine(layout, index, start, boxWidth, align);
			float x2 = caretXInLine(layout, index, end, boxWidth, align);
			if (x1 > x2)
			{
				std::swap(x1, x2);
			}
			if (x2 - x1 <= 0.0F)
			{
				continue;
			}

			rects.push_back(
				Rectangle{
					.X = x1,
					.Y = static_cast<float>(i) * layout.LineHeight,
					.Width = x2 - x1,
					.Height = layout.LineHeight,
				}
			);
		}

		return rects;
	}
}
