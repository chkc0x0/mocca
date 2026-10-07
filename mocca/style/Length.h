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

		Length() = default;
		Length(float pixels) : Value(pixels) {};
		Length(LengthUnit unit, float value) : Unit(unit), Value(value) {};
	};
}