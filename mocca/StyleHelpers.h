#pragma once
#include "Style.h"

namespace mocca::styles
{
	inline auto px(float value) -> Length
	{
		return {value};
	}

	inline auto percent(float value) -> Length
	{
		return {LengthUnit::Percent, value};
	}

	namespace detail
	{
		template <typename T>
		inline auto edgeHorizontal(T left, T right) -> DeclaredEdges<T>
		{
			return {.Left = left, .Right = right};
		}

		template <typename T>
		inline auto edgeVertical(T top, T bottom) -> DeclaredEdges<T>
		{
			return {.Top = top, .Bottom = bottom};
		}

		template <typename T>
		inline auto edgeHorizontal(T both) -> DeclaredEdges<T>
		{
			return {.Left = both, .Right = both};
		}

		template <typename T>
		inline auto edgeVertical(T both) -> DeclaredEdges<T>
		{
			return {.Top = both, .Bottom = both};
		}

		template <typename T>
		inline auto edgeHorizontal(StyleValue<T> left, StyleValue<T> right)
			-> DeclaredEdges<T>
		{
			return {.Left = left, .Right = right};
		}

		template <typename T>
		inline auto edgeVertical(StyleValue<T> top, StyleValue<T> bottom)
			-> DeclaredEdges<T>
		{
			return {.Top = top, .Bottom = bottom};
		}

		template <typename T>
		inline auto edgeHorizontal(StyleValue<T> both) -> DeclaredEdges<T>
		{
			return {.Left = both, .Right = both};
		}

		template <typename T>
		inline auto edgeVertical(StyleValue<T> both) -> DeclaredEdges<T>
		{
			return {.Top = both, .Bottom = both};
		}

		template <typename T> inline auto edgeAll(T all) -> DeclaredEdges<T>
		{
			return {all, all, all, all};
		}

		template <typename T>
		inline auto edgeAll(StyleValue<T> all) -> DeclaredEdges<T>
		{
			return {all, all, all, all};
		}
	}

#undef mc_stylePropertyEdge
#define mc_stylePropertyEdge(name, type)                                       \
	inline auto name(type all)                                                 \
	{                                                                          \
		return detail::edgeAll<type>(all);                                     \
	}                                                                          \
	inline auto name##Horizontal(type left, type right)                        \
	{                                                                          \
		return detail::edgeHorizontal<type>(left, right);                      \
	}                                                                          \
	inline auto name##Horizontal(type both)                                    \
	{                                                                          \
		return detail::edgeHorizontal<type>(both);                             \
	}                                                                          \
	inline auto name##Vertical(type top, type bottom)                          \
	{                                                                          \
		return detail::edgeVertical<type>(top, bottom);                        \
	}                                                                          \
	inline auto name##Vertical(type both)                                      \
	{                                                                          \
		return detail::edgeVertical<type>(both);                               \
	}                                                                          \
	inline auto name(StyleValue<type> all)                                     \
	{                                                                          \
		return detail::edgeAll<type>(all);                                     \
	}                                                                          \
	inline auto name##Horizontal(                                              \
		StyleValue<type> left,                                                 \
		StyleValue<type> right                                                 \
	)                                                                          \
	{                                                                          \
		return detail::edgeHorizontal<type>(left, right);                      \
	}                                                                          \
	inline auto name##Horizontal(StyleValue<type> both)                        \
	{                                                                          \
		return detail::edgeHorizontal<type>(both);                             \
	}                                                                          \
	inline auto name##Vertical(StyleValue<type> top, StyleValue<type> bottom)  \
	{                                                                          \
		return detail::edgeVertical<type>(top, bottom);                        \
	}                                                                          \
	inline auto name##Vertical(StyleValue<type> both)                          \
	{                                                                          \
		return detail::edgeVertical<type>(both);                               \
	}

	mc_propertyEdges
#undef mc_stylePropertyEdge
}