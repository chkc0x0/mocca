#pragma once
#include "StyleProperty.h"
#include "TransitionType.h"
#include "styles.def"
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <vector>

namespace mocca::styles
{
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
		auto internTransition(const TransitionType& type) -> uint32_t;
		auto resolveTransition(uint32_t ref) -> const TransitionType&;
	}
}
