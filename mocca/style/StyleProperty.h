#pragma once
#include "styles.def"

namespace mocca::styles
{
	enum class StyleProperty : char
	{
		None,
#undef mc_styleProperty
#define mc_styleProperty(name, ...) name,
		mc_layoutProperties mc_renderProperties
#undef mc_styleProperty
			StylePropertyCount,
	};

	enum class StylePropertyGroup : char
	{
		LayoutSizing,
		LayoutSpacing,
		LayoutFlex,
		LayoutPosition,
		Render,
		Inherited,
	};
}
