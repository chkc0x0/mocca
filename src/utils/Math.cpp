#include "utils/Math.h"

namespace mocca
{
	inline auto clampRadius(float radius, const Rectangle& rect) -> float
	{
		float limit = std::min(rect.Width, rect.Height) * 0.5F;
		return std::max(0.0F, std::min(radius, limit));
	}
	
	auto RRect::Inset(const EdgeSizes& border) const -> RRect
	{
		RRect inner{};
		inner.Rect.X = Rect.X + border.Left;
		inner.Rect.Y = Rect.Y + border.Top;
		inner.Rect.Width = Rect.Width - border.Left - border.Right;
		inner.Rect.Height = Rect.Height - border.Top - border.Bottom;

		inner.Radii.TopLeft = clampRadius(
			Radii.TopLeft - std::max(border.Top, border.Left),
			inner.Rect
		);
		inner.Radii.TopRight = clampRadius(
			Radii.TopRight - std::max(border.Top, border.Right),
			inner.Rect
		);
		inner.Radii.BottomRight = clampRadius(
			Radii.BottomRight - std::max(border.Bottom, border.Right),
			inner.Rect
		);
		inner.Radii.BottomLeft = clampRadius(
			Radii.BottomLeft - std::max(border.Bottom, border.Left),
			inner.Rect
		);

		return inner;
	}
}