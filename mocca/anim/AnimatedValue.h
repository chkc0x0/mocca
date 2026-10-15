#pragma once
#include "Curve.h"
#include "Interpolate.h"
#include "Spring.h"
#include <algorithm>

namespace mocca::anim
{
	template <typename T> struct AnimatedValue
	{
	public:
		T From{};
		T Value{};
		T Target{};
		float Elapsed = 0.0F;
		float Duration = 0.0F;
		float Progress = 0.0F;
		Curve Easing{};
		SpringState Spring{};
		bool Active = false;
		bool Initialized = false;

		[[nodiscard]] auto Settled() const -> bool
		{
			if (!Active)
			{
				return true;
			}
			if (Easing.Kind == CurveKind::Spring)
			{
				return springSettled(Progress, 1.0F, Spring);
			}
			return Elapsed >= Duration;
		}

		[[nodiscard]] auto Simulated() const -> T
		{
			if (!Active)
			{
				return Value;
			}
			if (Easing.Kind == CurveKind::Spring)
			{
				return interpolate(From, Target, Progress);
			}
			float t = Duration > 0.0F ? std::min(Elapsed / Duration, 1.0F) : 1.0F;
			return interpolate(From, Target, sample(Easing, t));
		}

		auto Start(const T& target, float duration, const Curve& easing) -> void
		{
			From = Value;
			Target = target;
			Duration = duration;
			Easing = easing;
			Elapsed = 0.0F;
			Progress = 0.0F;
			Spring = {};
			Active = true;
		}

		auto Jump(const T& value) -> void
		{
			From = value;
			Value = value;
			Target = value;
			Elapsed = 0.0F;
			Progress = 0.0F;
			Spring = {};
			Active = false;
			Initialized = true;
		}

		auto Advance(float dt) -> void
		{
			if (!Active)
			{
				return;
			}

			if (Easing.Kind == CurveKind::Spring)
			{
				Progress = springStep(Progress, 1.0F, Spring, Easing, dt);
				if (Settled())
				{
					Progress = 1.0F;
					Active = false;
				}
				Value = interpolate(From, Target, Progress);
				return;
			}

			Elapsed += dt;
			float t = Duration > 0.0F ? std::min(Elapsed / Duration, 1.0F) : 1.0F;
			Value = interpolate(From, Target, sample(Easing, t));
			if (Elapsed >= Duration)
			{
				Value = Target;
				Active = false;
			}
		}
	};
}
