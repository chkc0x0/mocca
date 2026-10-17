#pragma once

namespace mocca
{
	template <class T> struct Range
	{
		T Lo{}, Hi{};
		constexpr Range() = default;
		constexpr Range(T v) : Lo(v), Hi(v)
		{
		}
		constexpr Range(T l, T h) : Lo(l), Hi(h)
		{
		}
		constexpr auto Contains(T v) const -> bool
		{
			return v >= Lo && v <= Hi;
		}
		[[nodiscard]] constexpr auto IsVariable() const -> bool
		{
			return Lo != Hi;
		}
	};
}