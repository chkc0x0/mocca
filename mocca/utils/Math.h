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

		[[nodiscard]] auto Intersects(const Rectangle& other) const -> bool
		{
			return X < other.X + other.Width && other.X < X + Width &&
				   Y < other.Y + other.Height && other.Y < Y + Height;
		}

		[[nodiscard]] auto Union(const Rectangle& other) const -> Rectangle
		{
			if (IsEmpty())
			{
				return other;
			}
			if (other.IsEmpty())
			{
				return *this;
			}

			float x1 = std::min(X, other.X);
			float y1 = std::min(Y, other.Y);
			float x2 = std::max(X + Width, other.X + other.Width);
			float y2 = std::max(Y + Height, other.Y + other.Height);

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

	struct CornerRadii
	{
	public:
		float TopLeft;
		float TopRight;
		float BottomRight;
		float BottomLeft;

		[[nodiscard]] auto HasRadius() const -> bool
		{
			return TopLeft > 0 || TopRight > 0 || BottomRight > 0 ||
				   BottomLeft > 0;
		}
	};

	struct EdgeSizes
	{
	public:
		float Top;
		float Right;
		float Bottom;
		float Left;
	};

	struct RRect
	{
	public:
		Rectangle Rect;
		CornerRadii Radii;

		[[nodiscard]] auto Inset(const EdgeSizes& border) const -> RRect;
		[[nodiscard]] auto IsEmpty() const -> bool
		{
			return Rect.IsEmpty();
		}
	};
}