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

	inline auto em(float value) -> Length
	{
		return {LengthUnit::Em, value};
	}

	inline auto rem(float value) -> Length
	{
		return {LengthUnit::Rem, value};
	}
}