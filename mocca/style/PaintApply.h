#pragma once
#include "Length.h"
#include "styles.def"
#include "gfx/Paint.h"
#include "gfx/Texture.h"
#include "utils/Color.h"
#include "utils/Math.h"
#include <vector>

namespace mocca::styles
{
	struct ComputedStyle;

	struct BoxVisual
	{
	public:
		bool HasFill = false;
		gfx::Paint Fill{};
		bool HasImage = false;
		std::vector<gfx::TextureId> Images;
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

	auto resolveBoxVisual(const ComputedStyle& style, float w, float h)
		-> BoxVisual;
}

namespace mocca::styles::detail
{
	auto resolveLengthPx(const Length& value, float basis) -> float;
}

namespace mocca::styles::detail::applying::paint
{
	struct PaintContext
	{
	public:
		float Width;
		float Height;
	};

#undef mc_styleProperty
#define mc_styleProperty(name, type, inherits, initial, ...)                   \
	void apply##name(                                                          \
		BoxVisual& d,                                                          \
		const type& value,                                                     \
		const PaintContext& ctx                                                \
	);
	mc_renderPropertiesReset
#undef mc_styleProperty

	void applyBorderTop(
		BoxVisual& d,
		const Length& value,
		const PaintContext& ctx
	);
	void applyBorderRight(
		BoxVisual& d,
		const Length& value,
		const PaintContext& ctx
	);
	void applyBorderBottom(
		BoxVisual& d,
		const Length& value,
		const PaintContext& ctx
	);
	void applyBorderLeft(
		BoxVisual& d,
		const Length& value,
		const PaintContext& ctx
	);

	void normalizeRadius(BoxVisual& d, float width, float height);
}
