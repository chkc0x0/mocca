#pragma once

namespace mocca
{
	enum class LengthUnit : char
	{
		Pixels,
		Percent,
	};

	struct Length
	{
	public:
		LengthUnit Unit{};
		float Value{0};

		constexpr Length() = default;
		constexpr Length(float pixels) : Value(pixels) {};
		constexpr Length(LengthUnit unit, float value) : Unit(unit), Value(value) {};

		constexpr auto operator==(const Length&) const -> bool = default;
	};
}