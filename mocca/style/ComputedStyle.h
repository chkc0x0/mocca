#pragma once
#include "StyleSet.h"
#include "StyleProperty.h"
#include "StyleValue.h"
#include "StyleEntry.h"
#include "Length.h"
#include "utils/Color.h"

namespace mocca::styles
{
	template <StyleProperty T> struct PropTraits
	{
	};

	class ComputedStyle
	{
	public:
#undef mc_styleProperty
#define mc_styleProperty(name, type, inh, initial, ...) type name = initial;
		struct LayoutSizingProps
		{
			mc_layoutSizingProps
		} LayoutSizing;

		struct LayoutSpacingProps
		{
			mc_layoutSpacingProps
		} LayoutSpacing;

		struct LayoutFlexProps
		{
			mc_layoutFlexProps
		} LayoutFlex;

		struct LayoutPositionProps
		{
			mc_layoutPosProps
		} LayoutPosition;

		struct RenderProps
		{
			mc_renderPropertiesReset
		} Render;

		// maybe switch to per category later
		// if it's a problem
		struct InheritedProps
		{
			mc_layoutPropertiesInh mc_renderPropertiesInh
		} Inherited;
#undef mc_styleProperty

		void Apply(const StyleEntry& e, ComputedStyle& parent);
		static auto Cascade(const StyleSet& s, ComputedStyle& parent)
			-> ComputedStyle;

		template <StyleProperty P>
		[[nodiscard]] auto Get() const -> const PropTraits<P>::Value&;

		template <StyleProperty P> void Set(const PropTraits<P>::Value& value)
		{
			_ref<P>() = value;
		}
		
		void Blend(
			StyleProperty prop,
			const StyleEntry& from,
			const StyleEntry& goal,
			float t
		);

	private:
		template <StyleProperty P> auto _ref() -> PropTraits<P>::Value&;
	};

#undef mc_styleProperty
#define mc_cat_(a, b) a##b
#define mc_cat(a, b) mc_cat_(a, b)
#define mc_styleProperty(name, type, inherits, initial, ...)                   \
	template <> struct PropTraits<StyleProperty::name>                         \
	{                                                                          \
		using Value = type;                                                    \
		static constexpr bool Inherits = inherits;                             \
		static constexpr auto Initial() -> Value                               \
		{                                                                      \
			return initial;                                                    \
		}                                                                      \
		static constexpr StylePropertyGroup Group =                            \
			StylePropertyGroup::mc_styleGroup;                                 \
		static constexpr auto Handle = &ComputedStyle::mc_styleGroup;          \
		static constexpr auto Field =                                          \
			&ComputedStyle::mc_cat(mc_styleGroup, Props)::name;                \
	};
#define mc_styleGroup LayoutSizing
	mc_layoutSizingProps
#undef mc_styleGroup
#define mc_styleGroup LayoutSpacing
		mc_layoutSpacingProps
#undef mc_styleGroup
#define mc_styleGroup LayoutFlex
			mc_layoutFlexProps
#undef mc_styleGroup
#define mc_styleGroup LayoutPosition
				mc_layoutPosProps
#undef mc_styleGroup
#define mc_styleGroup Render
					mc_renderPropertiesReset
#undef mc_styleGroup
#define mc_styleGroup Inherited
						mc_inheritProperties
#undef mc_styleGroup
#undef mc_styleProperty
#undef mc_cat_
#undef mc_cat

	template <StyleProperty P>
	auto ComputedStyle::Get() const -> const PropTraits<P>::Value&
	{
		using L = PropTraits<P>;
		return (this->*L::Handle).*L::Field;
	}

	template <StyleProperty P>
	auto ComputedStyle::_ref() -> PropTraits<P>::Value&
	{
		using L = PropTraits<P>;
		return (this->*L::Handle).*L::Field;
	}

	auto getDefaultStyle() -> ComputedStyle&;
}
