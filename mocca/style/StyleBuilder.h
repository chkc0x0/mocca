#pragma once
#include "StyleEntry.h"
#include "StyleProperty.h"
#include "StyleValue.h"
#include "FontStyleTypes.h"
#include "FontVariants.h"
#include "Transition.h"
#include "StyleEnums.h"
#include "Length.h"
#include "utils/Color.h"
#include <cassert>
#include <concepts>
#include <cstdint>
#include <initializer_list>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

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

		constexpr auto encodeStyleEntry(StyleProperty prop, uint8_t d)
			-> StyleEntry
		{
			return {
				.Prop = prop,
				.Type = StyleValueType::AuxKeyword,
				.Aux = d,
			};
		};

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
				.Type = StyleValueType::Enum,
				.Payload = {.Enum = static_cast<uint8_t>(a)},
			};
		}

		constexpr auto encodeStyleEntry(StyleProperty prop, FlexDirections f)
			-> StyleEntry
		{
			return {
				.Prop = prop,
				.Type = StyleValueType::Enum,
				.Payload = {.Enum = static_cast<uint8_t>(f)},
			};
		}

		constexpr auto encodeStyleEntry(StyleProperty prop, WrappingType w)
			-> StyleEntry
		{
			return {
				.Prop = prop,
				.Type = StyleValueType::Enum,
				.Payload = {.Enum = static_cast<uint8_t>(w)},
			};
		}

		constexpr auto encodeStyleEntry(StyleProperty prop, Justification j)
			-> StyleEntry
		{
			return {
				.Prop = prop,
				.Type = StyleValueType::Enum,
				.Payload = {.Enum = static_cast<uint8_t>(j)},
			};
		}

		constexpr auto encodeStyleEntry(StyleProperty prop, LayoutDirections l)
			-> StyleEntry
		{
			return {
				.Prop = prop,
				.Type = StyleValueType::Enum,
				.Payload = {.Enum = static_cast<uint8_t>(l)},
			};
		}

		constexpr auto encodeStyleEntry(StyleProperty prop, OverflowType o)
			-> StyleEntry
		{
			return {
				.Prop = prop,
				.Type = StyleValueType::Enum,
				.Payload = {.Enum = static_cast<uint8_t>(o)},
			};
		}

		constexpr auto encodeStyleEntry(StyleProperty prop, DisplayType d)
			-> StyleEntry
		{
			return {
				.Prop = prop,
				.Type = StyleValueType::Enum,
				.Payload = {.Enum = static_cast<uint8_t>(d)},
			};
		}

		constexpr auto encodeStyleEntry(StyleProperty prop, TextAlignment d)
			-> StyleEntry
		{
			return {
				.Prop = prop,
				.Type = StyleValueType::Enum,
				.Payload = {.Enum = static_cast<uint8_t>(d)},
			};
		}

		template <class T>
		constexpr auto encodeStyleEntry(StyleProperty prop, EnumValue<T> k)
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
					encodeStyleEntry(
						static_cast<StyleProperty>(
							static_cast<uint8_t>(prop) + 1
						),
						r
					),
					encodeStyleEntry(
						static_cast<StyleProperty>(
							static_cast<uint8_t>(prop) + 2
						),
						b
					),
					encodeStyleEntry(
						static_cast<StyleProperty>(
							static_cast<uint8_t>(prop) + 3
						),
						l
					),
				},
			};
		}

		constexpr auto encodeStyleEntry(StyleProperty prop, std::uint32_t ref)
			-> StyleEntry
		{
			return {
				.Prop = prop,
				.Type = StyleValueType::Handle,
				.Payload = {.Handle = ref},
			};
		}

		constexpr auto encodeStyleEntry(StyleProperty prop, FontStyleType style)
			-> StyleEntry
		{
			return {
				.Prop = prop,
				.Type = StyleValueType::FontStyle,
				.Payload = {.FontStyle = style},
			};
		}

		constexpr auto encodeStyleEntry(StyleProperty prop, LineHeightType k)
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

		inline auto makeTransition(const TransitionType& type) -> StyleEntry
		{
			return encodeStyleEntry(
				StyleProperty::Transition,
				detail::internTransition(type)
			);
		}

		inline auto makeFontFamily(const FontFamilyType& family) -> StyleEntry
		{
			return encodeStyleEntry(
				StyleProperty::FontFamily,
				detail::internFontFamily(family)
			);
		}

		inline auto makeFontVariations(const FontVariationsType& variations)
			-> StyleEntry
		{
			return encodeStyleEntry(
				StyleProperty::FontVariationSettings,
				detail::internFontVariations(variations)
			);
		}

		inline auto makeFontFeatures(const FontFeaturesType& features)
			-> StyleEntry
		{
			return encodeStyleEntry(
				StyleProperty::FontFeatureSettings,
				detail::internFontFeatures(features)
			);
		}

		inline void appendFontFeatures(
			std::vector<FontFeaturesType::Value>& out,
			const FontFeaturesType& features
		)
		{
			out.insert(out.end(), features.Axes.begin(), features.Axes.end());
		}

		inline void appendFontFeatures(
			std::vector<FontFeaturesType::Value>& out,
			const FontFeaturesType::Value& value
		)
		{
			out.push_back(value);
		}

		inline void appendFontFeatures(
			std::vector<FontFeaturesType::Value>& out,
			std::initializer_list<FontFeaturesType::Value> values
		)
		{
			out.insert(out.end(), values.begin(), values.end());
		}

		template <class T> auto encodeStyleEntry(T) -> StyleEntry = delete;
	}

#undef mc_styleProperty
#define mc_styleProperty(name, type, ...)                                      \
	constexpr auto name(type val) -> StyleEntry                                \
	{                                                                          \
		return detail::encodeStyleEntry(StyleProperty::name, val);             \
	}
	mc_textProperties mc_layoutProperties mc_renderProperties
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

		inline auto BackgroundImage(const gfx::TextureId& id) -> StyleEntry
	{
		return detail::encodeStyleEntry(
			StyleProperty::BackgroundImage,
			detail::internBackgroundImage(id)
		);
	}

		constexpr auto ms(float milliseconds) -> float
	{
		return milliseconds / 1000.0F;
	}

	template <class... Names>
		requires(sizeof...(Names) > 0) &&
				(std::convertible_to<Names, std::string_view> && ...)
	inline auto FontFamily(Names&&... names) -> StyleEntry
	{
		FontFamilyType family;
		family.Families.reserve(sizeof...(Names));
		(family.Families.emplace_back(std::forward<Names>(names)), ...);
		return detail::makeFontFamily(family);
	}

	inline auto FontVariationSettings(
		std::initializer_list<FontVariationsType::Value> axes
	) -> StyleEntry
	{
		return detail::makeFontVariations(FontVariationsType{axes});
	}

	inline auto FontFeatureSettings(
		std::initializer_list<FontFeaturesType::Value> axes
	) -> StyleEntry
	{
		return detail::makeFontFeatures(FontFeaturesType{axes});
	}

	// merge partial feature sets (raw pairs, variant shorthand groups, or
	// whole lists) into one interned FontFeatureSettings entry. later
	// sources override earlier ones tag-by-tag.
	template <class... Sets>
		requires(sizeof...(Sets) > 0)
	inline auto FontFeatureSettings(const Sets&... sets) -> StyleEntry
	{
		std::vector<FontFeaturesType::Value> merged;
		(detail::appendFontFeatures(merged, sets), ...);
		return detail::makeFontFeatures(FontFeaturesType{merged});
	}

	// TODO: make it per property properly later
	inline auto Transition(
		StyleProperty prop,
		float duration,
		anim::Curve curve = anim::EaseInOut
	) -> StyleEntry
	{
		return detail::makeTransition(
			TransitionType{
				.Props = animatableBit(prop),
				.Duration = duration,
				.Curve = curve,
			}
		);
	}

	inline auto Transition(StyleProperty prop, anim::Curve curve) -> StyleEntry
	{
		return Transition(prop, TransitionType{}.Duration, curve);
	}

	inline auto Transition(
		std::initializer_list<StyleProperty> props,
		float duration,
		anim::Curve curve = anim::EaseInOut
	) -> StyleEntry
	{
		std::uint32_t mask = 0;
		for (StyleProperty prop : props)
		{
			mask |= animatableBit(prop);
		}
		return detail::makeTransition(
			TransitionType{.Props = mask, .Duration = duration, .Curve = curve}
		);
	}

	inline auto Transition(
		std::initializer_list<StyleProperty> props,
		anim::Curve curve
	) -> StyleEntry
	{
		return Transition(props, TransitionType{}.Duration, curve);
	}

	inline auto Transition(float duration, anim::Curve curve = anim::EaseInOut)
		-> StyleEntry
	{
		return detail::makeTransition(
			TransitionType{
				.Props = AllProperties,
				.Duration = duration,
				.Curve = curve,
			}
		);
	}

	inline auto Transition(anim::Curve curve) -> StyleEntry
	{
		return Transition(TransitionType{}.Duration, curve);
	}

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
		return detail::edges(
			StyleProperty::BorderRadiusTopLeft,
			tl,
			tr,
			bl,
			br
		);
	}

	// i somehow got the order wrong lol
	// but i already applied it everywhere so i wont change it
	// ^ (it was trbl instead of tlbr)
}
