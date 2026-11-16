#pragma once
#include "anim/Curve.h"
#include "core/Identifiers.h"
#include "style/StyleProperty.h"
#include <cstddef>
#include <cstdint>
#include <functional>
#include <ranges>
#include <vector>

namespace mocca::styles
{
	struct PropertySet
	{
	public:
		std::uint64_t Lo = 0;
		std::uint64_t Hi = 0;

		static constexpr auto Set(StyleProperty prop) -> PropertySet
		{
			if (prop == StyleProperty::All)
			{
				return {.Lo = ~0ULL, .Hi = ~0ULL};
			}

			auto index = static_cast<std::size_t>(prop);
			if (index < 64)
			{
				return {.Lo = 1ULL << index, .Hi = 0};
			}
			return {.Lo = 0, .Hi = 1ULL << (index - 64)};
		}

		[[nodiscard]] constexpr auto Test(StyleProperty prop) const -> bool
		{
			if (prop == StyleProperty::All)
			{
				return IsAll();
			}

			auto index = static_cast<std::size_t>(prop);
			if (index < 64)
			{
				return (Lo & (1ULL << index)) != 0;
			}
			return (Hi & (1ULL << (index - 64))) != 0;
		}

		[[nodiscard]] constexpr auto Intersects(const PropertySet& other) const
			-> bool
		{
			return (Lo & other.Lo) != 0 || (Hi & other.Hi) != 0;
		}

		[[nodiscard]] constexpr auto IsAll() const -> bool
		{
			return Lo == ~0ULL && Hi == ~0ULL;
		}

		constexpr auto operator|(const PropertySet& other) const -> PropertySet
		{
			return {.Lo = Lo | other.Lo, .Hi = Hi | other.Hi};
		}

		constexpr auto operator==(const PropertySet&) const -> bool = default;

		constexpr static auto LayoutAffecting() -> PropertySet
		{
			constexpr std::uint8_t LayoutGroups =
				GroupBit(StylePropertyGroup::LayoutSizing) |
				GroupBit(StylePropertyGroup::LayoutSpacing) |
				GroupBit(StylePropertyGroup::LayoutFlex) |
				GroupBit(StylePropertyGroup::LayoutPosition);

			PropertySet set{};
			for (std::size_t i = 1; i < StylePropertyCount; ++i)
			{
				if ((GroupBitsOf[i] & LayoutGroups) != 0)
				{
					set = set | PropertySet::Set(static_cast<StyleProperty>(i));
				}
			}

			return set | PropertySet::Set(StyleProperty::LayoutDirection);
		}
	};

	struct TransitionType
	{
	public:
		PropertySet Props{};
		float Duration = 0.2F;
		anim::Curve Curve{};

		constexpr auto operator==(const TransitionType&) const
			-> bool = default;
	};

	struct TransitionList
	{
	public:
		std::vector<TransitionType> Specs;

		[[nodiscard]] constexpr auto Find(const PropertySet& propSet) const
			-> const TransitionType*
		{
			const TransitionType* all = nullptr;
			for (const auto& Spec : std::views::reverse(Specs))
			{
				if (Spec.Props.IsAll())
				{
					if (all == nullptr)
					{
						all = &Spec;
					}
					continue;
				}

				if (Spec.Props.Intersects(propSet))
				{
					return &Spec;
				}
			}
			return all;
		}

		[[nodiscard]] auto FlipTiming() const -> std::optional<TransitionType>
		{
			constexpr PropertySet Layout = PropertySet::LayoutAffecting();

			for (const auto& spec : std::views::reverse(Specs))
			{
				if (spec.Props.IsAll() || spec.Props.Intersects(Layout))
				{
					return spec;
				}
			}
			return std::nullopt;
		}

		constexpr auto operator==(const TransitionList&) const
			-> bool = default;
	};

	inline constexpr PropertySet AllProperties{
		.Lo = ~0ULL,
		.Hi = ~0ULL,
	};
}

namespace std
{
	template <> struct hash<mocca::anim::Curve>
	{
		auto operator()(const mocca::anim::Curve& c) const noexcept -> size_t
		{
			size_t h = std::hash<uint8_t>{}(static_cast<uint8_t>(c.Kind));
			mocca::detail::hashCombine(h, std::hash<float>{}(c.P1));
			mocca::detail::hashCombine(h, std::hash<float>{}(c.P2));
			mocca::detail::hashCombine(h, std::hash<float>{}(c.P3));
			mocca::detail::hashCombine(h, std::hash<float>{}(c.P4));
			return h;
		}
	};

	template <> struct hash<mocca::styles::TransitionType>
	{
		auto operator()(const mocca::styles::TransitionType& t) const noexcept
			-> size_t
		{
			size_t h = std::hash<std::uint64_t>{}(t.Props.Lo);
			mocca::detail::hashCombine(
				h,
				std::hash<std::uint64_t>{}(t.Props.Hi)
			);
			mocca::detail::hashCombine(h, std::hash<float>{}(t.Duration));
			mocca::detail::hashCombine(
				h,
				std::hash<mocca::anim::Curve>{}(t.Curve)
			);
			return h;
		}
	};

	template <> struct hash<mocca::styles::TransitionList>
	{
		auto operator()(const mocca::styles::TransitionList& l) const noexcept
			-> size_t
		{
			size_t h = std::hash<size_t>{}(l.Specs.size());
			for (const auto& spec : l.Specs)
			{
				mocca::detail::hashCombine(
					h,
					std::hash<mocca::styles::TransitionType>{}(spec)
				);
			}
			return h;
		}
	};
}
