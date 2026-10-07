#pragma once
#include "Length.h"

namespace mocca::styles
{
	inline auto px(float value) -> Length
	{
		return {value};
	}

	inline auto percent(float value) -> Length
	{
		return {LengthUnit::Percent, value};
	}
}