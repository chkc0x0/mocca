#include "text/FontMatcher.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace mocca::mtext
{
	namespace
	{
		struct Rank
		{
			uint8_t Tier;
			float Dist;
			auto operator<=>(const Rank&) const = default;
		};

		constexpr Rank WorstRank{
			.Tier = 255,
			.Dist = std::numeric_limits<float>::infinity(),
		};

		auto Nearest(const Range<float>& r, float v) -> float
		{
			return std::clamp(v, r.Lo, r.Hi);
		}

		auto StretchRank(const Range<float>& r, float d) -> Rank
		{
			if (r.Contains(d))
			{
				return {.Tier = 0, .Dist = 0};
			}
			float v = Nearest(r, d);
			bool narrower = v < d;
			bool narrowerFirst = d <= 100.0F;
			return {
				.Tier = static_cast<uint8_t>(narrower == narrowerFirst ? 1 : 2),
				.Dist = std::abs(v - d),
			};
		}

		auto WeightRank(const Range<float>& r, float d) -> Rank
		{
			if (r.Contains(d))
			{
				return {.Tier = 0, .Dist = 0};
			}
			float v = Nearest(r, d);
			float dist = std::abs(v - d);

			if (d >= 400.0F && d <= 500.0F)
			{
				if (v > d && v <= 500.0F)
				{
					return {.Tier = 1, .Dist = dist};
				}
				if (v < d)
				{
					return {.Tier = 2, .Dist = dist};
				}
				return {.Tier = 3, .Dist = dist};
			}
			if (d < 400.0F)
			{
				return {
					.Tier = static_cast<uint8_t>(v < d ? 1 : 2),
					.Dist = dist,
				};
			}
			return {
				.Tier = static_cast<uint8_t>(v > d ? 1 : 2),
				.Dist = dist,
			};
		}

		auto StyleRank(const FaceProps& p, const FontRequest& q) -> Rank
		{
			static constexpr uint8_t Order[3][3] = {
				{0, 2, 1}, // normal
				{2, 0, 1}, // italic
				{2, 1, 0}, // oblique
			};

			uint8_t tier = Order[int(q.Style)][int(p.Style)];
			float dist = (p.Style == FontStyleEnum::Oblique)
							 ? std::abs(Nearest(p.Slant, q.Slant) - q.Slant)
							 : 0.0F;
			return {.Tier = tier, .Dist = dist};
		}
	}

	auto FontMatcher::Best(
		std::span<const FontDescriptor*> candidates,
		const FontRequest& q
	) -> FontMatchResult
	{
		auto stretchOf = [&](const FontDescriptor* d) -> Rank
		{ return StretchRank(d->Stretch, q.Stretch); };
		auto styleOf = [&](const FontDescriptor* d) -> Rank
		{ return StyleRank(*d, q); };
		auto weightOf = [&](const FontDescriptor* d) -> Rank
		{ return WeightRank(d->Weight, q.Weight); };

		Rank s = WorstRank;
		Rank y = WorstRank;
		Rank w = WorstRank;

		for (const auto* d : candidates)
		{
			s = std::min(s, stretchOf(d));
		}
		for (const auto* d : candidates)
		{
			if (stretchOf(d) == s)
			{
				y = std::min(y, styleOf(d));
			}
		}
		for (const auto* d : candidates)
		{
			if (stretchOf(d) == s && styleOf(d) == y)
			{
				w = std::min(w, weightOf(d));
			}
		}

		for (size_t i = 0; i < candidates.size(); i++)
		{
			const auto* d = candidates[i];
			if (stretchOf(d) != s || styleOf(d) != y || weightOf(d) != w)
			{
				continue;
			}

			FontMatchResult m;
			m.Index = i;
			m.Weight = Nearest(d->Weight, q.Weight);
			m.Stretch = Nearest(d->Stretch, q.Stretch);
			m.Slant = (d->Style == FontStyleEnum::Oblique &&
					   q.Style == FontStyleEnum::Oblique)
						  ? Nearest(d->Slant, q.Slant)
						  : 0.0F;
			return m;
		}
		return {};
	}
}
