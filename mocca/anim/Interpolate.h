#pragma once
#include "style/Length.h"
#include "utils/Color.h"
#include <algorithm>
#include <type_traits>

namespace mocca::anim
{
	inline auto lerp(float a, float b, float t) -> float
	{
		return a + ((b - a) * t);
	}

	inline auto lerp(const Color& a, const Color& b, float t) -> Color
	{
		return Color::MixOklab(a, b, std::clamp(t, 0.0F, 1.0F));
	}

	inline auto lerp(const Length& a, const Length& b, float t) -> Length
	{
		float value = a.Value + ((b.Value - a.Value) * t);
		return {a.Unit, value < 0.0F ? 0.0F : value};
	}

	template <typename T>
	inline constexpr bool IsInterpolatable =
		std::is_same_v<T, float> || std::is_same_v<T, Color> ||
		std::is_same_v<T, Length>;

	template <typename T>
	auto interpolate(const T& a, const T& b, float t) -> T
	{
		if constexpr (IsInterpolatable<T>)
		{
			return lerp(a, b, t);
		}
		else
		{
			return t < 0.5F ? a : b;
		}
	}
}
