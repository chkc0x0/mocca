#pragma once
#include "anim/Curve.h"

namespace mocca::anim::presets::cubic
{
	inline constexpr Curve Anticipate = bezier(1, -0.40, 0.35, 0.95);
	inline constexpr Curve QuickOut = bezier(0, 0, 0.20, 1);
	inline constexpr Curve OvershootOut = bezier(0.17, 0.89, 0.32, 1.27);
	inline constexpr Curve SwiftOut = bezier(0.17, 0.89, 0.32, 1.10);
	inline constexpr Curve SnappyOut = bezier(0.19, 1, 0.22, 1);
	inline constexpr Curve InOut = bezier(0.42, 0, 0.58, 1);
	inline constexpr Curve InQuad = bezier(0.55, 0.09, 0.68, 0.53);
	inline constexpr Curve InCubic = bezier(0.55, 0.06, 0.68, 0.19);
	inline constexpr Curve InQuart = bezier(0.9, 0.03, 0.69, 0.22);
	inline constexpr Curve InQuint = bezier(0.76, 0.05, 0.85, 0.06);
	inline constexpr Curve InExpo = bezier(0.95, 0.05, 0.80, 0.04);
	inline constexpr Curve InCirc = bezier(0.60, 0.04, 0.98, 0.34);
	inline constexpr Curve OutQuad = bezier(0.25, 0.46, 0.45, 0.94);
	inline constexpr Curve OutCubic = bezier(0.21, 0.61, 0.35, 1);
	inline constexpr Curve OutQuart = bezier(0.17, 0.84, 0.44, 1);
	inline constexpr Curve OutQuint = bezier(0.23, 1, 0.32, 1);
	inline constexpr Curve OutExpo = bezier(0.19, 1, 0.22, 1);
	inline constexpr Curve OutCirc = bezier(0.07, 0.82, 0.17, 1);
	inline constexpr Curve InOutQuad = bezier(0.46, 0.03, 0.52, 0.95);
	inline constexpr Curve InOutCubic = bezier(0.65, 0.04, 0.35, 1);
	inline constexpr Curve InOutQuart = bezier(0.77, 0, 0.17, 1);
	inline constexpr Curve InOutQuint = bezier(0.86, 0, 0.07, 1);
	inline constexpr Curve InOutExpo = bezier(1, 0, 0, 1);
	inline constexpr Curve InOutCirc = bezier(0.79, 0.14, 0.15, 0.86);
	inline constexpr Curve InOutBase = bezier(0.25, 0.1, 0.25, 1);
	inline constexpr Curve In = bezier(0.42, 0, 1, 1);
	inline constexpr Curve Out = bezier(0, 0, 0.58, 1);
	inline constexpr Curve Linear = bezier(0, 0, 1, 1);
}

namespace mocca::anim::presets::springs
{
	inline constexpr Curve Buoyant = spring(900, 80, 10);
	inline constexpr Curve Elegant = spring(150, 19, 1.2);
	inline constexpr Curve Bob = spring(131.1, 2.3, 0.1);
	inline constexpr Curve Fling = spring(800, 80, 4);
	inline constexpr Curve Swift = spring(280, 18, 0.3);
	inline constexpr Curve Float = spring(290, 15, 2);
	inline constexpr Curve Slow = spring(26.7, 4.1, 0.2);
	inline constexpr Curve Snap = spring(320, 20, 0.4);
	inline constexpr Curve Stern = spring(550, 30, 1.2);
	inline constexpr Curve Boingoingoing = spring(1000, 1.5, 0.1);
	inline constexpr Curve Lively = spring(250, 12, 1);
	inline constexpr Curve Smooth = spring(170, 26, 1);
	inline constexpr Curve Crisp = spring(600, 49, 1);
	inline constexpr Curve Glide = spring(100, 24, 1);
	inline constexpr Curve Gentle = spring(300, 24, 1);
	inline constexpr Curve Heavy = spring(200, 30, 3);
	inline constexpr Curve Jelly = spring(200, 6, 1);
}