#include "anim/Curve.h"
#include <cmath>

namespace mocca::anim
{
	auto sample(const Curve& curve, float x) -> float
	{
		if (curve.Kind != CurveKind::Bezier)
		{
			return x;
		}

		float t = x;
		for (int i = 0; i < 8; ++i)
		{
			float slope = detail::cubicSlope(t, curve.P1, curve.P3);
			if (std::abs(slope) < 1e-6F)
			{
				break;
			}
			float error = detail::cubicAt(t, curve.P1, curve.P3) - x;
			t -= error / slope;
		}

		if (t < 0.0F || t > 1.0F)
		{
			float lo = 0.0F;
			float hi = 1.0F;
			for (int i = 0; i < 24; ++i)
			{
				float mid = 0.5F * (lo + hi);
				if (detail::cubicAt(mid, curve.P1, curve.P3) < x)
				{
					lo = mid;
				}
				else
				{
					hi = mid;
				}
			}
			t = 0.5F * (lo + hi);
		}

		return detail::cubicAt(t, curve.P2, curve.P4);
	}
}