#pragma once
#include "Length.h"
#include <cstdint>
#include <variant>

namespace mocca::styles
{
	namespace
	{
		inline constexpr uint8_t Auto = 0;
		inline constexpr uint8_t MaxContent = 1;
		inline constexpr uint8_t FitContent = 2;
		inline constexpr uint8_t Stretch = 3;
		inline constexpr uint8_t Normal = 4;
	}

	template <typename T> using EnumValue = std::variant<T, uint8_t>;

	using SizingValue = EnumValue<Length>;
	using LineHeightType = std::variant<Length, float, uint8_t>;
}
