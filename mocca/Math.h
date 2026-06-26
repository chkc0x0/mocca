#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <numbers>

namespace mocca
{
	namespace detail
	{
		// ideally we would use the std::pow/cos/sin functions and such
		// but we don't have them in constexpr so...
		// and i also don't want to include gcem just for comp-time colors

		constexpr auto ipow(double base, int exp) -> double
		{
			double res = 1.0;
			while (exp > 0)
			{
				if (exp % 2 == 1)
				{
					res *= base;
				}
				base *= base;
				exp /= 2;
			}
			return res;
		}

		constexpr auto root12(double a) -> double
		{
			if (a == 0.0)
			{
				return 0.0;
			}

			double x_n = 1.0;
			double x_prev = 0.0;

			for (int i = 0; i < 50; ++i)
			{
				x_prev = x_n;
				x_n = ((11.0 * x_n) + (a / ipow(x_n, 11))) / 12.0;
				if (x_n == x_prev)
				{
					break;
				}
			}
			return x_n;
		}

		constexpr auto fmod(double numer, double denom) -> double
		{
			auto quot = static_cast<long long>(numer / denom);
			return numer - (quot * denom);
		}

		constexpr auto reduceAngle(double angle) -> double
		{
			double x = fmod(angle, std::numbers::pi * 2);
			if (x > std::numbers::pi)
			{
				x -= std::numbers::pi * 2;
			}
			if (x < -std::numbers::pi)
			{
				x += std::numbers::pi * 2;
			}
			return x;
		}

		constexpr auto sin(double angle) -> double
		{
			double x = reduceAngle(angle);
			double sum = x;
			double term = x;
			double x_squared = x * x;

			for (int i = 1; i <= 10; ++i)
			{
				term *= -x_squared / ((2 * i) * ((2 * i) + 1));
				sum += term;
			}
			return sum;
		}

		constexpr auto cos(double angle) -> double
		{
			double x = reduceAngle(angle);
			double sum = 1.0;
			double term = 1.0;
			double x_squared = x * x;

			for (int i = 1; i <= 10; ++i)
			{
				term *= -x_squared / (((2 * i) - 1) * (2 * i));
				sum += term;
			}
			return sum;
		}

		constexpr auto cbrt(double x) -> double
		{
			if (x == 0.0)
			{
				return 0.0;
			}
			if (x < 0.0)
			{
				return -cbrt(-x);
			}

			double current = x > 1.0 ? x : 1.0;
			double prev = 0.0;

			for (int i = 0; i < 50; ++i)
			{
				prev = current;
				current = ((2.0 * current) + (x / (current * current))) / 3.0;
				if (current == prev)
				{
					break;
				}
			}
			return current;
		}

		constexpr auto root5(double x) -> double
		{
			if (x == 0.0)
			{
				return 0.0;
			}

			double current = x > 1.0 ? x : 1.0;
			double prev = 0.0;

			for (int i = 0; i < 50; ++i)
			{
				prev = current;
				double t2 = current * current;
				double t4 = t2 * t2;
				current = ((4.0 * current) + (x / t4)) / 5.0;
				if (current == prev)
				{
					break;
				}
			}
			return current;
		}

		constexpr auto pow24(double x) -> double
		{
			if (x < 0.0)
			{
				return 0.0;
			}
			double x2 = x * x;
			return x2 * root5(x2);
		}

		constexpr auto oklabPow(double x) -> double
		{
			if (x < 0.0)
			{
				return 0.0;
			}
			return root12(ipow(x, 5));
		}

		struct OklchF
		{
			double L, C, H;
		};
		struct OklabF
		{
			double L, A, B;
		};

		constexpr static auto ToOklab(uint8_t rr, uint8_t gg, uint8_t bb)
			-> OklabF
		{
			auto linearize = [](double x) -> double
			{
				x /= 255.0;
				return x <= 0.04045 ? x / 12.92 : pow24((x + 0.055) / 1.055);
			};

			double r = linearize(rr);
			double g = linearize(gg);
			double b = linearize(bb);

			double l = (0.4122214708 * r) + (0.5363325363 * g) +
					   (0.0514459929 * b);
			double m = (0.2119034982 * r) + (0.6806995451 * g) +
					   (0.1073969566 * b);
			double s = (0.0883024619 * r) + (0.2817188376 * g) +
					   (0.6299787005 * b);

			double lc = cbrt(l);
			double mc = cbrt(m);
			double sc = cbrt(s);

			return {
				.L = (0.2104542553 * lc) + (0.7936177850 * mc) -
					 (0.0040720468 * sc),
				.A = (1.9779984951 * lc) - (2.4285922050 * mc) +
					 (0.4505937099 * sc),
				.B = (0.0259040371 * lc) + (0.7827717662 * mc) -
					 (0.8086757660 * sc),
			};
		}

		constexpr auto atanBackend(double x) -> double
		{
			return x *
				   (0.9999993329 +
					(x *
					 (-0.0000424564 +
					  (x *
					   (-0.3323562304 +
						(x *
						 (0.0076380695 +
						  (x *
						   (0.1813401565 +
							(x *
							 (-0.0385317544 +
							  (x * (-0.0818228308 +
									(x * (0.0416972049 +
										  (x * (-0.0084478446)))))))))))))))));
		}

		constexpr auto constexprAbs(double v) -> double
		{
			return v < 0.0 ? -v : v;
		}

		constexpr auto atan(double x) -> double
		{
			if (x > 1.0)
			{
				return (std::numbers::pi / 2) - atanBackend(1.0 / x);
			}
			if (x < -1.0)
			{
				return -(std::numbers::pi / 2) - atanBackend(1.0 / x);
			}
			if (x < 0.0)
			{
				return -atanBackend(-x);
			}
			return atanBackend(x);
		}

		constexpr auto atan2(double y, double x) -> double
		{
			if (x == 0.0)
			{
				if (y > 0.0)
				{
					return (std::numbers::pi / 2);
				}
				if (y < 0.0)
				{
					return -(std::numbers::pi / 2);
				}
				return 0.0;
			}

			double abs_y = constexprAbs(y);
			double abs_x = constexprAbs(x);
			double angle = 0.0;

			if (abs_y < abs_x)
			{
				angle = atanBackend(abs_y / abs_x);
			}
			else
			{
				angle = (std::numbers::pi / 2) - atanBackend(abs_x / abs_y);
			}

			if (x < 0.0)
			{
				if (y >= 0.0)
				{
					angle = std::numbers::pi - angle;
				}
				else
				{
					angle = -std::numbers::pi + angle;
				}
			}
			else
			{
				if (y < 0.0)
				{
					angle = -angle;
				}
			}

			return angle;
		}

		constexpr static auto ToOklch(uint8_t rr, uint8_t gg, uint8_t bb)
			-> OklchF
		{
			auto [l, a, b] = ToOklab(rr, gg, bb);
			double c = std::sqrt((a * a) + (b * b));
			double h = atan2(b, a) * (180.0 / std::numbers::pi);
			if (h < 0.0)
			{
				h += 360.0;
			}
			return {l, c, h};
		}
	}

	struct Color
	{
	public:
		uint8_t R = 255;
		uint8_t G = 255;
		uint8_t B = 255;
		uint8_t A = 255;

		constexpr Color() = default;
		constexpr Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255)
			: R(r), G(g), B(b), A(a) {};

		// Reading uint32_t as hex color.
		// Convention: values that fit in 24 bits are RRGGBB (alpha=255).
		// Values > 0xFFFFFF are RRGGBBAA.
		// NOTE: This is ambiguous for RGBA values where R=0x00
		// (e.g. 0x00FF0080 looks like a 24-bit value).
		// Users should use the R,G,B,A constructor for those.
		constexpr Color(uint32_t hex)
		{
			if (hex <= 0xFFFFFF)
			{
				R = (hex >> 16) & 0xFF;
				G = (hex >> 8) & 0xFF;
				B = hex & 0xFF;
				A = 255;
			}
			else
			{
				R = (hex >> 24) & 0xFF;
				G = (hex >> 16) & 0xFF;
				B = (hex >> 8) & 0xFF;
				A = hex & 0xFF;
			}
		}

		constexpr static auto Hsl(double h, double s, double l, double a = 1.0)
			-> Color
		{
			h = std::max(0.0, std::min(360.0, h));
			s = std::max(0.0, std::min(1.0, s));
			l = std::max(0.0, std::min(1.0, l));
			a = std::clamp(a, 0.0, 1.0);

			if (s == 0.0)
			{
				int gray = static_cast<int>(std::round(l * 255.0));
				uint8_t alpha = static_cast<uint8_t>(std::max(0.0, std::min(255.0, floor((a * 255.0) + 0.5))));
				return Color{
					static_cast<uint8_t>(gray),
					static_cast<uint8_t>(gray),
					static_cast<uint8_t>(gray),
					alpha
				};
			}

			double chroma = (1.0 - std::abs((2.0 * l) - 1.0)) * s;

			double hPrime = h / 60.0;
			double x = chroma * (1.0 - std::abs(std::fmod(hPrime, 2.0) - 1.0));

			double r1 = 0.0;
			double g1 = 0.0;
			double b1 = 0.0;

			if (hPrime >= 0.0 && hPrime < 1.0)
			{
				r1 = chroma;
				g1 = x;
				b1 = 0.0;
			}
			else if (hPrime >= 1.0 && hPrime < 2.0)
			{
				r1 = x;
				g1 = chroma;
				b1 = 0.0;
			}
			else if (hPrime >= 2.0 && hPrime < 3.0)
			{
				r1 = 0.0;
				g1 = chroma;
				b1 = x;
			}
			else if (hPrime >= 3.0 && hPrime < 4.0)
			{
				r1 = 0.0;
				g1 = x;
				b1 = chroma;
			}
			else if (hPrime >= 4.0 && hPrime < 5.0)
			{
				r1 = x;
				g1 = 0.0;
				b1 = chroma;
			}
			else if (hPrime >= 5.0 && hPrime <= 6.0)
			{
				r1 = chroma;
				g1 = 0.0;
				b1 = x;
			}

			double m = l - (chroma / 2.0);

			int r = static_cast<int>(std::round((r1 + m) * 255.0));
			int g = static_cast<int>(std::round((g1 + m) * 255.0));
			int b = static_cast<int>(std::round((b1 + m) * 255.0));

			uint8_t alpha = static_cast<uint8_t>(std::max(0.0, std::min(255.0, floor((a * 255.0) + 0.5))));
			return Color{
				static_cast<uint8_t>(std::clamp(r, 0, 255)),
				static_cast<uint8_t>(std::clamp(g, 0, 255)),
				static_cast<uint8_t>(std::clamp(b, 0, 255)),
				alpha
			};
		}

		constexpr static auto Hsv(double h, double s, double v, double a = 1.0)
			-> Color
		{
			double r;
			double g;
			double b;
			if (s == 0.0)
			{
				r = g = b = v;
			}
			else
			{
				double hn = h / 60.0;
				int i = static_cast<int>(hn) % 6;
				double f = hn - static_cast<int>(hn);
				double p = v * (1.0 - s);
				double q = v * (1.0 - (s * f));
				double t = v * (1.0 - (s * (1.0 - f)));
				switch (i)
				{
				case 0:
					r = v;
					g = t;
					b = p;
					break;
				case 1:
					r = q;
					g = v;
					b = p;
					break;
				case 2:
					r = p;
					g = v;
					b = t;
					break;
				case 3:
					r = p;
					g = q;
					b = v;
					break;
				case 4:
					r = t;
					g = p;
					b = v;
					break;
				default:
					r = v;
					g = p;
					b = q;
					break;
				}
			}

			return {
				static_cast<uint8_t>(floor((r * 255.0) + 0.5)),
				static_cast<uint8_t>(floor((g * 255.0) + 0.5)),
				static_cast<uint8_t>(floor((b * 255.0) + 0.5)),
				static_cast<uint8_t>(floor((a * 255.0) + 0.5))
			};
		}

		constexpr static auto
		Oklab(double l, double a, double b, double alpha = 1.0) -> Color
		{
			double ll = l + (0.3963377774 * a) + (0.2158037573 * b);
			double mm = l - (0.1055613458 * a) - (0.0638541728 * b);
			double ss = l - (0.0894841775 * a) - (1.2914855480 * b);

			double lc = ll * ll * ll;
			double mc = mm * mm * mm;
			double sc = ss * ss * ss;

			double r = (4.0767416621 * lc) - (3.3077115913 * mc) +
					   (0.2309699292 * sc);
			double g = (-1.2684380046 * lc) + (2.6097574011 * mc) -
					   (0.3413193965 * sc);
			double b_ = (-0.0041960863 * lc) - (0.7034186147 * mc) +
						(1.7076147010 * sc);

			// Linear sRGB -> gamma sRGB
			auto gamma = [](double x) -> double
			{
				x = x < 0.0 ? 0.0 : (x > 1.0 ? 1.0 : x);
				return x <= 0.0031308 ? 12.92 * x
									  : (1.055 * detail::oklabPow(x)) - 0.055;
			};

			return {
				static_cast<uint8_t>(floor((gamma(r) * 255.0) + 0.5)),
				static_cast<uint8_t>(floor((gamma(g) * 255.0) + 0.5)),
				static_cast<uint8_t>(floor((gamma(b_) * 255.0) + 0.5)),
				static_cast<uint8_t>(floor((alpha * 255.0) + 0.5))
			};
		}

		constexpr static auto
		Oklch(double l, double c, double h, double alpha = 1.0) -> Color
		{
			double hrad = h * (std::numbers::pi / 180.0);
			double a_ = c * detail::cos(hrad);
			double b_ = c * detail::sin(hrad);
			return Oklab(l, a_, b_, alpha);
		}

		constexpr static auto
		Hwb(double h, double w, double b, double alpha = 1.0) -> Color
		{
			double sum = w + b;
			if (sum > 1.0)
			{
				w /= sum;
				b /= sum;
			}

			double v = 1.0 - b;
			double s = (v == 0.0) ? 0.0 : 1.0 - (w / v);
			return Hsv(h, s, v, alpha);
		}

		constexpr static auto Mix(Color a, Color b, double t) -> Color
		{
			auto lerp = [](uint8_t x, uint8_t y, double t) -> uint8_t
			{ return static_cast<uint8_t>(floor(x + ((y - x) * t) + 0.5)); };
			return {
				lerp(a.R, b.R, t),
				lerp(a.G, b.G, t),
				lerp(a.B, b.B, t),
				lerp(a.A, b.A, t)
			};
		}

		[[nodiscard]] constexpr auto Lightened(double amount) const -> Color
		{
			auto [l, c, h] = detail::ToOklch(R, G, B);
			l = l + amount < 0.0 ? 0.0 : (l + amount > 1.0 ? 1.0 : l + amount);
			return Oklch(l, c, h, A / 255.0);
		}

		[[nodiscard]] constexpr auto Darkened(double amount) const -> Color
		{
			return Lightened(-amount);
		}

		[[nodiscard]] constexpr auto Saturated(double amount) const -> Color
		{
			auto [l, c, h] = detail::ToOklch(R, G, B);
			c = c + amount < 0.0 ? 0.0 : c + amount;
			return Oklch(l, c, h, A / 255.0);
		}

		[[nodiscard]] constexpr auto HueRotated(double degrees) const -> Color
		{
			auto [l, c, h] = detail::ToOklch(R, G, B);
			return Oklch(
				l,
				c,
				detail::fmod(h + degrees + 360.0, 360.0),
				A / 255.0
			);
		}

		constexpr static auto MixOklab(Color a, Color b, double t) -> Color
		{
			auto la = detail::ToOklab(a.R, a.G, a.B);
			auto lb = detail::ToOklab(b.R, b.G, b.B);
			auto lerp = [](double x, double y, double t) -> double
			{ return x + ((y - x) * t); };
			return Oklab(
				lerp(la.L, lb.L, t),
				lerp(la.A, lb.A, t),
				lerp(la.B, lb.B, t),
				(a.A + ((b.A - a.A) * t)) / 255.0
			);
		}

		[[nodiscard]] constexpr auto WithAlpha(double a) const -> Color
		{
			return {R, G, B, static_cast<uint8_t>(floor((a * 255.0) + 0.5))};
		}

		[[nodiscard]] constexpr auto WithAlpha8(uint8_t a) const -> Color
		{
			return {R, G, B, a};
		}

		[[nodiscard]] constexpr auto ToRgba() const -> uint32_t
		{
			return (static_cast<uint32_t>(R) << 24) |
				   (static_cast<uint32_t>(G) << 16) |
				   (static_cast<uint32_t>(B) << 8) | (static_cast<uint32_t>(A));
		}

		constexpr auto operator==(const Color&) const -> bool = default;
		constexpr auto operator<=>(const Color&) const
			-> std::strong_ordering = default;
	};

	namespace colors
	{
		inline constexpr Color Red = 0xFFFFFFFF;
	}

	struct Vector2
	{
		float X;
		float Y;
	};

	struct Rectangle
	{
		Vector2 Position;
		float Width;
		float Height;
	};
}