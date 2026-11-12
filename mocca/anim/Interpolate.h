#pragma once
#include "style/Length.h"
#include "utils/Color.h"
#include "utils/Math.h"
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

	inline auto lerp(const Vector2& a, const Vector2& b, float t) -> Vector2
	{
		return {
			.X = a.X + ((b.X - a.X) * t),
			.Y = a.Y + ((b.Y - a.Y) * t),
		};
	}

	inline auto lerp(
		const styles::Length& a,
		const styles::Length& b,
		float t,
		bool& discrete
	) -> styles::Length
	{
		if (a.Unit != b.Unit)
		{
			discrete = true;
			return t < 0.5F ? a : b;
		}

		return styles::Length{
			a.Unit,
			a.Value + ((b.Value - a.Value) * t),
		};
	}

	inline auto lerp(
		const styles::Vector2Length& a,
		const styles::Vector2Length& b,
		float t
	) -> styles::Vector2Length
	{
		bool discrete = false;
		styles::Vector2Length result{
			.X = lerp(a.X, b.X, t, discrete),
			.Y = lerp(a.Y, b.Y, t, discrete),
		};

		if (discrete)
		{
			return t < 0.5F ? a : b;
		}

		return result;
	}

	template <typename T>
	inline constexpr bool IsInterpolatable = std::is_same_v<T, float> ||
											 std::is_same_v<T, Color> ||
											 std::is_same_v<T, Vector2> ||
											 std::is_same_v<T, styles::Length> ||
											 std::is_same_v<T, styles::Vector2Length>;

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
