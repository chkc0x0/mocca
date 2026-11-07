#pragma once
#include "gfx/Paint.h"
#include "gfx/Texture.h"
#include "text/TextGeometry.h"
#include "utils/Color.h"
#include "utils/Math.h"
#include <vector>

namespace mocca::gfx
{
	struct BoxDecoration
	{
	public:
		bool HasFill = false;
		Paint Fill{};
		bool HasImage = false;
		std::vector<TextureId> Images;
		CornerRadii Radius{};
		EdgeSizes Border{};
		Color BorderColor = colors::Transparent;
		bool HasBorder = false;
		bool SizeDependent = false;

		[[nodiscard]] auto IsEmpty() const -> bool
		{
			return !HasFill && !HasImage && !HasBorder;
		}
	};

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

		[[nodiscard]] auto operator==(const TextDecoration&) const
			-> bool = default;
	};

	struct CaretDecoration
	{
		uint32_t Offset = 0;
		mtext::CaretAffinity Affinity = mtext::CaretAffinity::Downstream;
		struct Color Color = colors::Black;
		float Width = 1.5F;
		uint32_t BlinkPeriodMs = 530;
		uint64_t BlinkResetToken = 0;

		[[nodiscard]] auto operator==(const CaretDecoration&) const
			-> bool = default;
	};
}
