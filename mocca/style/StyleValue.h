#pragma once
#include "Length.h"
#include "style/FontStyleTypes.h"
#include "style/TransitionType.h"
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

	namespace detail
	{
		auto internTransition(const TransitionType& type) -> uint32_t;
		auto resolveTransition(uint32_t ref) -> const TransitionType&;

		auto internFontFamily(const FontFamilyType& family) -> uint32_t;
		auto resolveFontFamily(uint32_t ref) -> const FontFamilyType&;

		auto internFontVariations(const FontVariationsType& variations)
			-> uint32_t;
		auto resolveFontVariations(uint32_t ref) -> const FontVariationsType&;

		auto internFontFeatures(const FontFeaturesType& features) -> uint32_t;
		auto resolveFontFeatures(uint32_t ref) -> const FontFeaturesType&;
	}
}
