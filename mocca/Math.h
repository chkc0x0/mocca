#pragma once
#include <cstdint>

namespace mocca
{
	struct Color
	{
	public:
		uint8_t R = 255;
		uint8_t G = 255;
		uint8_t B = 255;
		uint8_t A = 255;
	};

	struct Vector2
	{
		float X;
		float Y;
	};

	struct Rectangle
	{
		Vector2 Position;
		float Width;
		float Height;
	};
}