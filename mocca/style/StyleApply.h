#pragma once
#include "StyleValue.h"
#include "StyleEnums.h"
#include "Length.h"
#include "styles.def"
#include "utils/Color.h"
#include "yoga/YGNode.h"

namespace mocca::gfx
{
	struct BoxDecoration;
}

namespace mocca::styles::detail::applying::layout
{
#undef mc_styleProperty
#define mc_styleProperty(name, type, inherits, initial, ...)                   \
	void apply##name(YGNodeRef ref, type value);
	mc_layoutProperties
#undef mc_styleProperty
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
		gfx::BoxDecoration& d,                                                 \
		const type& value,                                                     \
		const PaintContext& ctx                                                \
	);
	mc_renderPropertiesReset
#undef mc_styleProperty

	void applyBorderTop(
		gfx::BoxDecoration& d,
		const Length& value,
		const PaintContext& ctx
	);
	void applyBorderRight(
		gfx::BoxDecoration& d,
		const Length& value,
		const PaintContext& ctx
	);
	void applyBorderBottom(
		gfx::BoxDecoration& d,
		const Length& value,
		const PaintContext& ctx
	);
	void applyBorderLeft(
		gfx::BoxDecoration& d,
		const Length& value,
		const PaintContext& ctx
	);

	void normalizeRadius(gfx::BoxDecoration& d, float width, float height);
}
