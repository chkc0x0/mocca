#pragma once
#include "StyleEntry.h"
#include "StyleProperty.h"
#include "StyleValue.h"
#include "StyleEnums.h"
#include "Length.h"
#include "utils/Color.h"
#include <cassert>
#include <type_traits>
#include <variant>

namespace mocca::styles
{
	namespace detail
	{
		constexpr auto encodeStyleEntry(StyleProperty prop, StyleKeyword k)
			-> StyleEntry
		{
			return {
				.Prop = prop,
				.Type = StyleValueType::Keyword,
				.Payload = {.Keyword = k},
			};
		}

		constexpr auto encodeStyleEntry(StyleProperty prop, Length l)
			-> StyleEntry
		{
			return {
				.Prop = prop,
				.Type = StyleValueType::Length,
				.Payload = {.Length = l},
			};
		}

		constexpr auto encodeStyleEntry(StyleProperty prop, Color c)
			-> StyleEntry
		{
			return {
				.Prop = prop,
				.Type = StyleValueType::Color,
				.Payload = {.Color = c},
			};
		}

		constexpr auto encodeStyleEntry(StyleProperty prop, float f)
			-> StyleEntry
		{
			return {
				.Prop = prop,
				.Type = StyleValueType::Value,
				.Payload = {.Float = f},
			};
		}

		constexpr auto encodeStyleEntry(StyleProperty prop, Alignment a)
			-> StyleEntry
		{
			return {
				.Prop = prop,
				.Type = StyleValueType::AuxKeyword,
				.Payload = {.Enum = static_cast<uint8_t>(a)},
			};
		}

		constexpr auto encodeStyleEntry(StyleProperty prop, FlexDirections f)
			-> StyleEntry
		{
			return {
				.Prop = prop,
				.Type = StyleValueType::AuxKeyword,
				.Payload = {.Enum = static_cast<uint8_t>(f)},
			};
		}

		constexpr auto encodeStyleEntry(StyleProperty prop, WrappingType w)
			-> StyleEntry
		{
			return {
				.Prop = prop,
				.Type = StyleValueType::AuxKeyword,
				.Payload = {.Enum = static_cast<uint8_t>(w)},
			};
		}

		constexpr auto encodeStyleEntry(StyleProperty prop, Justification j)
			-> StyleEntry
		{
			return {
				.Prop = prop,
				.Type = StyleValueType::AuxKeyword,
				.Payload = {.Enum = static_cast<uint8_t>(j)},
			};
		}

		constexpr auto encodeStyleEntry(StyleProperty prop, LayoutDirections l)
			-> StyleEntry
		{
			return {
				.Prop = prop,
				.Type = StyleValueType::AuxKeyword,
				.Payload = {.Enum = static_cast<uint8_t>(l)},
			};
		}

		constexpr auto encodeStyleEntry(StyleProperty prop, OverflowType o)
			-> StyleEntry
		{
			return {
				.Prop = prop,
				.Type = StyleValueType::AuxKeyword,
				.Payload = {.Enum = static_cast<uint8_t>(o)},
			};
		}

		constexpr auto encodeStyleEntry(StyleProperty prop, DisplayType d)
			-> StyleEntry
		{
			return {
				.Prop = prop,
				.Type = StyleValueType::AuxKeyword,
				.Payload = {.Enum = static_cast<uint8_t>(d)},
			};
		}

		template <class T>
		constexpr auto encodeStyleEntry(StyleProperty prop, AutoValue<T> k)
			-> StyleEntry
		{
			return std::visit(
				[prop](auto&& arg) -> StyleEntry
				{
					using V = std::decay_t<decltype(arg)>;

					if constexpr (std::is_same_v<V, uint8_t>)
					{
						assert(arg == 0);
						return {
							.Prop = prop,
							.Type = StyleValueType::AuxKeyword,
							.Aux = 0,
						};
					}
					else
					{
						return encodeStyleEntry(prop, arg);
					}
				},
				k
			);
		}

		constexpr auto encodeStyleEntry(StyleProperty prop, SizingValue k)
			-> StyleEntry
		{
			return std::visit(
				[prop](auto&& arg) -> StyleEntry
				{
					using V = std::decay_t<decltype(arg)>;

					if constexpr (std::is_same_v<V, uint8_t>)
					{
						return {
							.Prop = prop,
							.Type = StyleValueType::AuxKeyword,
							.Aux = arg,
						};
					}
					else
					{
						return encodeStyleEntry(prop, arg);
					}
				},
				k
			);
		}

		template <class V>
		constexpr auto edges(StyleProperty prop, V t, V r, V b, V l)
			-> StyleEntryGroup<4>
		{
			return {
				{
					encodeStyleEntry(prop, t),
					encodeStyleEntry((StyleProperty)((uint8_t)prop + 1), r),
					encodeStyleEntry((StyleProperty)((uint8_t)prop + 2), b),
					encodeStyleEntry((StyleProperty)((uint8_t)prop + 3), l),
				},
			};
		}

		template <class T> auto encodeStyleEntry(T) -> StyleEntry = delete;
	}

#undef mc_styleProperty
#define mc_styleProperty(name, type, ...)                                      \
	constexpr auto name(type val) -> StyleEntry                                \
	{                                                                          \
		return detail::encodeStyleEntry(StyleProperty::name, val);             \
	}
	mc_layoutProperties mc_renderProperties
#undef mc_styleProperty

#undef mc_stylePropertyEdge
#define mc_stylePropertyEdge(name, type)                                       \
	constexpr auto name(type a)                                                \
	{                                                                          \
		return detail::edges(StyleProperty::name##Top, a, a, a, a);            \
	}                                                                          \
	constexpr auto name(type v, type h)                                        \
	{                                                                          \
		return detail::edges(StyleProperty::name##Top, v, h, v, h);            \
	}                                                                          \
	constexpr auto name(type t, type b, type h)                                \
	{                                                                          \
		return detail::edges(StyleProperty::name##Top, t, h, b, h);            \
	}                                                                          \
	constexpr auto name(type t, type r, type b, type l)                        \
	{                                                                          \
		return detail::edges(StyleProperty::name##Top, t, l, b, r);            \
	}

		mc_propertyEdges
#undef mc_stylePropertyEdge

		constexpr auto BorderRadius(Length a)
	{
		return detail::edges(StyleProperty::BorderRadiusTopLeft, a, a, a, a);
	}

	constexpr auto BorderRadius(Length v, Length h)
	{
		return detail::edges(StyleProperty::BorderRadiusTopLeft, v, h, h, v);
	}

	constexpr auto BorderRadius(Length t, Length b, Length h)
	{
		return detail::edges(StyleProperty::BorderRadiusTopLeft, t, b, b, h);
	}

	constexpr auto BorderRadius(Length tl, Length tr, Length br, Length bl)
	{
		return detail::edges(StyleProperty::BorderRadiusTopLeft, tl, tr, bl, br);
	}

	// i somehow got the order wrong lol
	// but i already applied it everywhere so i wont change it
	// ^ (it was trbl instead of tlbr)
}
