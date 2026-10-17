#pragma once
#include <cstdint>

namespace mocca::anim
{
	enum class CurveKind : std::uint8_t
	{
		Linear,
		Bezier,
		Spring,
	};

	struct Curve
	{
	public:
		CurveKind Kind = CurveKind::Bezier;
		
		float P1 = 0.25F;
		float P2 = 0.10F;
		float P3 = 0.25F;
		float P4 = 1.00F;

		constexpr auto operator==(const Curve&) const -> bool = default;
	};

	inline constexpr Curve LinearCurve{.Kind=CurveKind::Linear, .P1=0.0F, .P2=0.0F, .P3=1.0F, .P4=1.0F};
	inline constexpr Curve EaseIn{.Kind=CurveKind::Bezier, .P1=0.42F, .P2=0.0F, .P3=1.0F, .P4=1.0F};
	inline constexpr Curve EaseOut{.Kind=CurveKind::Bezier, .P1=0.0F, .P2=0.0F, .P3=0.58F, .P4=1.0F};
	inline constexpr Curve EaseInOut{.Kind=CurveKind::Bezier, .P1=0.42F, .P2=0.0F, .P3=0.58F, .P4=1.0F};

	constexpr auto bezier(float x1, float y1, float x2, float y2) -> Curve
	{
		return {.Kind=CurveKind::Bezier, .P1=x1, .P2=y1, .P3=x2, .P4=y2};
	}

	constexpr auto spring(float stiffness, float damping, float mass = 1.0F)
		-> Curve
	{
		return {.Kind=CurveKind::Spring, .P1=stiffness, .P2=damping, .P3=mass, .P4=0.0F};
	}

	namespace detail
	{
		constexpr auto cubicAt(float t, float a1, float a2) -> float
		{
			float u = 1.0F - t;
			return (3.0F * u * u * t * a1) + (3.0F * u * t * t * a2) +
				   (t * t * t);
		}

		constexpr auto cubicSlope(float t, float a1, float a2) -> float
		{
			float u = 1.0F - t;
			return (3.0F * u * u * a1) + (6.0F * u * t * (a2 - a1)) +
				   (3.0F * t * t * (1.0F - a2));
		}
	}

	auto sample(const Curve& curve, float x) -> float;
}