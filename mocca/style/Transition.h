#pragma once
#include "StyleProperty.h"
#include "anim/Curve.h"
#include "styles.def"
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <vector>

namespace mocca::styles
{
	struct TransitionType
	{
	public:
		std::uint32_t Props = 0;
		float Duration = 0.2F;
		anim::Curve Curve{};

		constexpr auto operator==(const TransitionType&) const
			-> bool = default;
	};

	inline constexpr std::uint32_t AllProperties = 0xFFFFFFFFU;

	inline constexpr StyleProperty AnimatablePropertyList[] = {
#define mc_animatable(name) StyleProperty::name,
		mc_animatableProperties
#undef mc_animatable
	};

	inline constexpr std::size_t AnimatablePropertyCount = std::size(
		AnimatablePropertyList
	);

	template <StyleProperty P> constexpr auto animatableIndex() -> std::size_t
	{
		for (std::size_t i = 0; i < AnimatablePropertyCount; ++i)
		{
			if (AnimatablePropertyList[i] == P)
			{
				return i;
			}
		}
		return AnimatablePropertyCount;
	}

	template <StyleProperty P> constexpr auto propertyBit() -> std::uint32_t
	{
		std::size_t index = animatableIndex<P>();
		return index < 32 ? (1U << index) : 0U;
	}

	constexpr auto animatableBit(StyleProperty prop) -> std::uint32_t
	{
		for (std::size_t i = 0; i < AnimatablePropertyCount; ++i)
		{
			if (AnimatablePropertyList[i] == prop)
			{
				return 1U << i;
			}
		}
		return 0U;
	}

	namespace detail
	{
		inline auto transitionTable() -> std::vector<TransitionType>&
		{
			static std::vector<TransitionType> table;
			return table;
		}

		inline auto internTransition(const TransitionType& type) -> std::uint32_t
		{
			auto& table = transitionTable();
			for (std::size_t i = 0; i < table.size(); ++i)
			{
				if (table[i] == type)
				{
					return static_cast<std::uint32_t>(i + 1);
				}
			}
			table.push_back(type);
			return static_cast<std::uint32_t>(table.size());
		}

		inline auto resolveTransition(std::uint32_t ref)
			-> const TransitionType&
		{
			static const TransitionType None{};
			auto& table = transitionTable();
			if (ref == 0 || ref > table.size())
			{
				return None;
			}
			return table[ref - 1];
		}
	}
}
