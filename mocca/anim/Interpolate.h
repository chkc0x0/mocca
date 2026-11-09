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
		float clamped = std::clamp(t, 0.0F, 1.0F);

		auto withAlpha8 = [](const Color& color, float alpha8) -> Color
		{ return color.WithAlpha8(static_cast<uint8_t>(alpha8 + 0.5F)); };

		if (a.A == 0 && b.A != 0)
		{
			if (clamped <= 0.0F)
			{
				return a;
			}
			if (clamped >= 1.0F)
			{
				return b;
			}
			return withAlpha8(b, clamped * static_cast<float>(b.A));
		}

		if (b.A == 0 && a.A != 0)
		{
			if (clamped <= 0.0F)
			{
				return a;
			}
			if (clamped >= 1.0F)
			{
				return b;
			}
			return withAlpha8(a, (1.0F - clamped) * static_cast<float>(a.A));
		}

		return Color::MixOklab(a, b, clamped);
	}

	inline auto lerp(const styles::Length& a, const styles::Length& b, float t)
		-> styles::Length
	{
		float value = a.Value + ((b.Value - a.Value) * t);
		return {a.Unit, value < 0.0F ? 0.0F : value};
	}

	template <typename T>
	inline constexpr bool IsInterpolatable = std::is_same_v<T, float> ||
											 std::is_same_v<T, Color> ||
											 std::is_same_v<T, styles::Length>;

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
