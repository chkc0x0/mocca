#include "style/ComputedStyle.h"
#include "core/Logger.h"
#include "anim/Interpolate.h"
#include "style/StyleHelpers.h"
#include "utils/ScopedProfiler.h"
#include <cmath>
#include <variant>

namespace mocca::styles
{
	namespace
	{
		auto LengthToPx(const Length& l, float emBasis) -> float
		{
			switch (l.Unit)
			{
			case LengthUnit::Percent:
				return l.Value / 100.0F * emBasis;
			case LengthUnit::Em:
				return l.Value * emBasis;
			case LengthUnit::Rem:
				return l.Value * RootFontSize;
			case LengthUnit::Pixels:
				break;
			}

			return l.Value;
		}

		void
		ResolveRelativeLengths(ComputedStyle& out, const ComputedStyle& parent)
		{
			float parentSize = parent.Inherited.FontSize.Value;

			Length& size = out.Inherited.FontSize;
			if (size.Unit != LengthUnit::Pixels)
			{
				size = Length{LengthToPx(size, parentSize)};
			}

			float ownSize = out.Inherited.FontSize.Value;

			auto& lineHeight = out.Inherited.LineHeight;
			if (auto* l = std::get_if<Length>(&lineHeight);
				l != nullptr && l->Unit != LengthUnit::Pixels)
			{
				lineHeight = Length{LengthToPx(*l, ownSize)};
			}

			for (EnumValue<Length>* spacing :
				 {&out.Inherited.LetterSpacing, &out.Inherited.WordSpacing})
			{
				if (auto* l = std::get_if<Length>(spacing);
					l != nullptr && l->Unit != LengthUnit::Pixels)
				{
					*spacing = Length{LengthToPx(*l, ownSize)};
				}
			}
		}

		template <class T> auto StyleEquals(const T& a, const T& b) -> bool
		{
			return a == b;
		}

		inline auto StyleEquals(float a, float b) -> bool
		{
			return a == b || (std::isnan(a) && std::isnan(b));
		}

		inline auto StyleEquals(const Length& a, const Length& b) -> bool
		{
			return a.Unit == b.Unit && StyleEquals(a.Value, b.Value);
		}

		template <class... Ts>
		auto
		StyleEquals(const std::variant<Ts...>& a, const std::variant<Ts...>& b)
			-> bool
		{
			if (a.index() != b.index())
			{
				return false;
			}

			return std::visit(
				[&](const auto& value) -> bool
				{
					using T = std::decay_t<decltype(value)>;
					return StyleEquals(value, std::get<T>(b));
				},
				a
			);
		}
	}

#undef mc_styleProperty
#define mc_styleProperty(name, ...) &&StyleEquals(name, b.name)
#define mc_groupEqual(fn, groupType, listMacro)                                \
	auto ComputedStyle::groupType::operator==(                                 \
		const ComputedStyle::groupType& b                                      \
	) const -> bool                                                                  \
	{                                                                          \
		return true listMacro;                                                 \
	}

	mc_groupEqual(layoutSizingEqual, LayoutSizingProps, mc_layoutSizingProps) mc_groupEqual(layoutSpacingEqual, LayoutSpacingProps, mc_layoutSpacingProps) mc_groupEqual(
		layoutFlexEqual,
		LayoutFlexProps,
		mc_layoutFlexProps
	) mc_groupEqual(layoutPositionEqual, LayoutPositionProps, mc_layoutPosProps) mc_groupEqual(renderEqual, RenderProps, mc_renderPropertiesReset) mc_groupEqual(inheritedEqual, InheritedProps, mc_inheritProperties)

#undef mc_groupEqual
#undef mc_styleProperty

		void ComputedStyle::Blend(
			StyleProperty prop,
			const StyleEntry& from,
			const StyleEntry& goal,
			float t
		)
	{
		switch (prop)
		{
#undef mc_animatable
#define mc_animatable(name)                                                    \
	case StyleProperty::name:                                                  \
		_ref<StyleProperty::name>() = anim::interpolate(                       \
			getStyleEntryValue<PropTraits<StyleProperty::name>::Value>(from),  \
			getStyleEntryValue<PropTraits<StyleProperty::name>::Value>(goal),  \
			t                                                                  \
		);                                                                     \
		break;
			mc_animatableProperties
#undef mc_animatable
				default : break;
		}
	}

	void ComputedStyle::Apply(const StyleEntry& e, ComputedStyle& parent)
	{
		mc_profileFunction();

#undef mc_styleProperty
#define mc_styleProperty(name, ...)                                            \
	case StyleProperty::name:                                                  \
		_ref<StyleProperty::name>() = e.Resolve(                               \
			parent._ref<StyleProperty::name>(),                                \
			PropTraits<StyleProperty::name>()                                  \
		);                                                                     \
		break;

		switch (e.Prop)
		{
			mc_textProperties mc_layoutProperties mc_renderProperties
				mc_miscProperties default
				: mc_error(
					  ErrorCode::InvalidArgument,
					  "how did we get to this style? ({})",
					  (int)e.Prop
				  );
			break;
		}
	}

	auto ComputedStyle::Cascade(const StyleSet& s, ComputedStyle& parent)
		-> ComputedStyle
	{
		ComputedStyle out;
		out.Inherited = parent.Inherited;

		for (const auto& e : s.Entries())
		{
			out.Apply(e, parent);
		}

		ResolveRelativeLengths(out, parent);

		return out;
	}

	auto getDefaultStyle() -> ComputedStyle&
	{
		// TODO: make this interned so it's just a bunch of handles instead of this mess
		static ComputedStyle style;
		return style;
	}

	auto ComputedStyle::ChangedGroups(
		const ComputedStyle& a,
		const ComputedStyle& b
	) -> std::uint8_t
	{
		std::uint8_t groups = 0;
		if (a.LayoutSizing != b.LayoutSizing)
		{
			groups |= GroupBit(StylePropertyGroup::LayoutSizing);
		}
		if (a.LayoutSpacing != b.LayoutSpacing)
		{
			groups |= GroupBit(StylePropertyGroup::LayoutSpacing);
		}
		if (a.LayoutFlex != b.LayoutFlex)
		{
			groups |= GroupBit(StylePropertyGroup::LayoutFlex);
		}
		if (a.LayoutPosition != b.LayoutPosition)
		{
			groups |= GroupBit(StylePropertyGroup::LayoutPosition);
		}
		if (a.Render != b.Render)
		{
			groups |= GroupBit(StylePropertyGroup::Render);
		}
		if (a.Inherited != b.Inherited)
		{
			groups |= GroupBit(StylePropertyGroup::Inherited);
		}
		return groups;
	}
}
