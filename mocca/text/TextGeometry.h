#pragma once
#include "style/StyleEnums.h"
#include "text/TextLayout.h"
#include "utils/Math.h"
#include <cstdint>
#include <vector>

namespace mocca::mtext
{
	enum class CaretAffinity : uint8_t
	{
		Downstream,
		Upstream,
	};

	struct TextPosition
	{
		uint32_t Offset = 0;
		CaretAffinity Affinity = CaretAffinity::Downstream;
	};

	struct TextCaret
	{
		uint32_t Line = 0;
		float X = 0;
		float Y = 0;
		float Height = 0;
	};

	class TextGeometry
	{
	public:
		[[nodiscard]] static auto PositionAtPoint(
			const TextLayout& layout,
			Vector2 point,
			float boxWidth,
			styles::TextAlignment align
		) -> TextPosition;

		[[nodiscard]] static auto CaretAt(
			const TextLayout& layout,
			TextPosition position,
			float boxWidth,
			styles::TextAlignment align
		) -> TextCaret;

		[[nodiscard]] static auto LineAtPoint(const TextLayout& layout, float y)
			-> uint32_t;

		[[nodiscard]] static auto
		LineOf(const TextLayout& layout, TextPosition position) -> uint32_t;

		[[nodiscard]] static auto LineStart(
			const TextLayout& layout,
			TextPosition position
		) -> TextPosition;

		[[nodiscard]] static auto LineEnd(
			const TextLayout& layout,
			TextPosition position
		) -> TextPosition;

		[[nodiscard]] static auto SelectionRects(
			const TextLayout& layout,
			uint32_t anchor,
			uint32_t active,
			float boxWidth,
			styles::TextAlignment align
		) -> std::vector<Rectangle>;
	};
}
