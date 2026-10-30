#pragma once
#include "text/TextGeometry.h"
#include "utils/Color.h"
#include <cstdint>

namespace mocca::mtext
{
	enum class TextDecorationKind : uint8_t
	{
		Selection,
		Underline,
		Overline,
		Strikethrough,
	};

	struct TextDecoration
	{
		TextDecorationKind Kind = TextDecorationKind::Selection;
		uint32_t Start = 0;
		uint32_t End = 0;
		struct Color Color = {0, 0, 0, 64};
		float Thickness = 1.0F;

		[[nodiscard]] auto operator==(const TextDecoration&) const -> bool = default;
	};

	struct CaretDecoration
	{
		uint32_t Offset = 0;
		CaretAffinity Affinity = CaretAffinity::Downstream;
		struct Color Color = colors::Black;
		float Width = 1.5F;
		uint32_t BlinkPeriodMs = 530;
		uint64_t BlinkResetToken = 0;

		[[nodiscard]] auto operator==(const CaretDecoration&) const -> bool = default;
	};
}
