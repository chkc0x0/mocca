#pragma once
#include "StyleValue.h"
#include "StyleEnums.h"
#include "Length.h"
#include "styles.def"
#include "yoga/YGNode.h"

namespace mocca::styles::detail::applying::layout
{
#undef mc_styleProperty
#define mc_styleProperty(name, type, inherits, initial, ...)                   \
	void apply##name(YGNodeRef ref, type value);
	mc_layoutProperties
#undef mc_styleProperty
}
