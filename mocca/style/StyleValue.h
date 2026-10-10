#pragma once
#include "Length.h"
#include <cstdint>
#include <variant>

namespace mocca
{
	namespace
	{
		inline constexpr uint8_t Auto = 0;
		inline constexpr uint8_t MaxContent = 1;
		inline constexpr uint8_t FitContent = 2;
		inline constexpr uint8_t Stretch = 3;
	}

	template <typename T> using AutoValue = std::variant<T, uint8_t>;

	using SizingValue = std::variant<Length, uint8_t>;
}
