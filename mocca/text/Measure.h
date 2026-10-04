#pragma once
#include "yoga/YGNode.h"

namespace mocca::mtext::detail
{
	auto measureFunc(
		const YGNode* ref,
		float width,
		YGMeasureMode widthMode,
		float height,
		YGMeasureMode heightMode
	) -> YGSize;
}