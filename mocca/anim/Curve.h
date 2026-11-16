#pragma once
#include <cstdint>
#include <numbers>

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

	constexpr auto bezier(float x1, float y1, float x2, float y2) -> Curve
	{
		return {.Kind=CurveKind::Bezier, .P1=x1, .P2=y1, .P3=x2, .P4=y2};
	}

	constexpr auto spring(float stiffness, float damping, float mass = 1.0F)
		-> Curve
	{
		return {.Kind=CurveKind::Spring, .P1=stiffness, .P2=damping, .P3=mass, .P4=0.0F};
	}

	constexpr auto springPerceptual(float duration, float bounce = 0.0) -> Curve
	{
		float w = 2.0F * std::numbers::pi / duration;
		float stiffness = w * w;
		float damping = bounce >= 0.0
							? 4.0 * std::numbers::pi * (1.0 - bounce) / duration
							: 4.0 * std::numbers::pi /
								  (duration * (1.0 + bounce));
		return spring(stiffness, damping, 1.0);
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