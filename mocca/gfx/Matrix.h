#pragma once
#include "utils/Math.h"
#include <cmath>
#include <numbers>
#include <optional>

namespace mocca::gfx
{
	// not in utils bc its quite a special niche construct
	struct Matrix
	{
	public:
		float M[6] = {1.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F};

		static auto Identity() -> Matrix
		{
			return {};
		}

		static auto Translate(float dx, float dy) -> Matrix
		{
			return {1.0F, 0.0F, dx, 0.0F, 1.0F, dy};
		}

		static auto Scale(float sx, float sy) -> Matrix
		{
			return {sx, 0.0F, 0.0F, 0.0F, sy, 0.0F};
		}

		static auto Rotate(float degrees) -> Matrix
		{
			float radians = degrees * std::numbers::pi_v<float> / 180.0F;
			float c = std::cos(radians);
			float s = std::sin(radians);
			return {c, -s, 0.0F, s, c, 0.0F};
		}

		static auto Concat(const Matrix& a, const Matrix& b) -> Matrix
		{
			Matrix result{};
			result.M[0] = (a.M[0] * b.M[0]) + (a.M[1] * b.M[3]);
			result.M[1] = (a.M[0] * b.M[1]) + (a.M[1] * b.M[4]);
			result.M[2] = (a.M[0] * b.M[2]) + (a.M[1] * b.M[5]) + a.M[2];
			result.M[3] = (a.M[3] * b.M[0]) + (a.M[4] * b.M[3]);
			result.M[4] = (a.M[3] * b.M[1]) + (a.M[4] * b.M[4]);
			result.M[5] = (a.M[3] * b.M[2]) + (a.M[4] * b.M[5]) + a.M[5];
			return result;
		}

		[[nodiscard]] auto MapPoint(const Vector2& p) const -> Vector2
		{
			return {
				.X = (M[0] * p.X) + (M[1] * p.Y) + M[2],
				.Y = (M[3] * p.X) + (M[4] * p.Y) + M[5],
			};
		}

		[[nodiscard]] auto MapRect(const Rectangle& rect) const -> Rectangle
		{
			Vector2 a = MapPoint({.X = rect.X, .Y = rect.Y});
			Vector2 b = MapPoint({.X = rect.X + rect.Width, .Y = rect.Y});
			Vector2 c = MapPoint(
				{.X = rect.X, .Y = rect.Y + rect.Height}
			);
			Vector2 d = MapPoint(
				{.X = rect.X + rect.Width, .Y = rect.Y + rect.Height}
			);

			float minX = std::min({a.X, b.X, c.X, d.X});
			float minY = std::min({a.Y, b.Y, c.Y, d.Y});
			float maxX = std::max({a.X, b.X, c.X, d.X});
			float maxY = std::max({a.Y, b.Y, c.Y, d.Y});

			return {
				.X = minX,
				.Y = minY,
				.Width = maxX - minX,
				.Height = maxY - minY,
			};
		}

		[[nodiscard]] auto Invert() const -> std::optional<Matrix>
		{
			float det = (M[0] * M[4]) - (M[1] * M[3]);
			if (det == 0.0F)
			{
				return std::nullopt;
			}

			Matrix inv{};
			inv.M[0] = M[4] / det;
			inv.M[1] = -M[1] / det;
			inv.M[3] = -M[3] / det;
			inv.M[4] = M[0] / det;
			inv.M[2] = -((inv.M[0] * M[2]) + (inv.M[1] * M[5]));
			inv.M[5] = -((inv.M[3] * M[2]) + (inv.M[4] * M[5]));
			return inv;
		}

		[[nodiscard]] auto IsIdentity() -> bool
		{
			return M[0] == 1.0F && M[1] == 0.0F && M[2] == 0.0F &&
				   M[3] == 0.0F && M[4] == 1.0F && M[5] == 0.0F;
		}

		[[nodiscard]] auto IsTranslateOnly() -> bool
		{
			return M[0] == 1.0F && M[1] == 0.0F && M[3] == 0.0F && M[4] == 1.0F;
		}
	};
}
