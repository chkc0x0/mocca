#pragma once
#include "anim/Curve.h"
#include "core/Identifiers.h"
#include <cstdint>
#include <functional>
#include <vector>

namespace mocca::styles
{
	struct TransitionType
	{
	public:
		uint32_t Props = 0;
		float Duration = 0.2F;
		anim::Curve Curve{};

		constexpr auto operator==(const TransitionType&) const
			-> bool = default;
	};

	struct TransitionList
	{
	public:
		std::vector<TransitionType> Specs;

		constexpr auto operator==(const TransitionList&) const
			-> bool = default;
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
			size_t h = std::hash<uint32_t>{}(t.Props);
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
