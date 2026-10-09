#pragma once
#include "utils/Color.h"
#include "styles.def"
#include "yoga/YGNode.h"
#include <variant>
#include "StyleEnums.h"
#include "Length.h"

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

	namespace styles::detail::applying::layout
	{
#undef mc_styleProperty
#define mc_styleProperty(name, type, inherits, initial, ...)                   \
	void apply##name(YGNodeRef ref, type value);
		mc_layoutProperties
#undef mc_styleProperty
	}
}