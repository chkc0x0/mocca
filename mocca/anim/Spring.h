#pragma once
#include "Curve.h"
#include <algorithm>
#include <cmath>

namespace mocca::anim
{
	struct SpringState
	{
	public:
		float Velocity = 0.0F;
	};

	inline auto
	springStep(float value, float target, SpringState& state, const Curve& curve, float dt)
		-> float
	{
		float stiffness = curve.P1 > 0.0F ? curve.P1 : 180.0F;
		float damping = curve.P2 > 0.0F ? curve.P2 : 20.0F;
		float mass = curve.P3 > 0.0F ? curve.P3 : 1.0F;

		constexpr float MaxStep = 1.0F / 120.0F;
		float remaining = dt;
		while (remaining > 0.0F)
		{
			float h = std::min(remaining, MaxStep);
			float accel =
				((-stiffness * (value - target)) - (damping * state.Velocity)) / mass;
			state.Velocity += accel * h;
			value += state.Velocity * h;
			remaining -= h;
		}

		return value;
	}

	inline auto springSettled(float value, float target, const SpringState& state)
		-> bool
	{
		return std::abs(target - value) < 1e-3F &&
			   std::abs(state.Velocity) < 1e-3F;
	}
}
