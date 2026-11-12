#pragma once
#include "core/Identifiers.h"
#include <cstdint>
#include <functional>

namespace mocca::styles
{
	enum class LengthUnit : char
	{
		Pixels,
		Percent,
		Em,
		Rem,
	};

	struct Length
	{
	public:
		LengthUnit Unit{};
		float Value{0};

		constexpr Length() = default;
		constexpr Length(float pixels) : Value(pixels) {};
		constexpr Length(LengthUnit unit, float value)
			: Unit(unit), Value(value) {};

		constexpr auto operator==(const Length&) const -> bool = default;
	};

	struct Vector2Length
	{
		Length X;
		Length Y;

		constexpr auto operator==(const Vector2Length&) const
			-> bool = default;
	};
}

namespace std
{
	template <> struct hash<mocca::styles::Vector2Length>
	{
		auto operator()(const mocca::styles::Vector2Length& v) const noexcept
			-> size_t
		{
			size_t h = std::hash<float>{}(v.X.Value);
			mocca::detail::hashCombine(
				h,
				std::hash<uint8_t>{}(static_cast<uint8_t>(v.X.Unit))
			);
			mocca::detail::hashCombine(h, std::hash<float>{}(v.Y.Value));
			mocca::detail::hashCombine(
				h,
				std::hash<uint8_t>{}(static_cast<uint8_t>(v.Y.Unit))
			);
			return h;
		}
	};
}