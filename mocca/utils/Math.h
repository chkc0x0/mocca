#pragma once
#include <algorithm>

namespace mocca
{
	struct Vector2
	{
		float X;
		float Y;
	};

	struct IVec2
	{
		int X;
		int Y;
	};

	struct Rectangle
	{
		float X;
		float Y;
		float Width;
		float Height;

		[[nodiscard]] auto Intersect(const Rectangle& other) const -> Rectangle
		{
			float x1 = std::max(X, other.X);
			float y1 = std::max(Y, other.Y);
			float x2 = std::min(X + Width, other.X + other.Width);
			float y2 = std::min(Y + Height, other.Y + other.Height);

			if (x2 < x1 || y2 < y1)
			{
				return {.X = x1, .Y = y1, .Width = 0, .Height = 0};
			}

			return {.X = x1, .Y = y1, .Width = x2 - x1, .Height = y2 - y1};
		}

		[[nodiscard]] auto Contains(Vector2 p) const -> bool
		{
			return p.X >= X && p.X < X + Width && p.Y >= Y && p.Y < Y + Height;
		}

		[[nodiscard]] auto IsEmpty() const -> bool
		{
			return Width <= 0 || Height <= 0;
		}
	};
}