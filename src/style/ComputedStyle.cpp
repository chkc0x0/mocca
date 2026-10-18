#include "style/ComputedStyle.h"
#include "core/Logger.h"
#include "anim/Interpolate.h"
#include "style/StyleHelpers.h"
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

		void ResolveRelativeLengths(
			ComputedStyle& out,
			const ComputedStyle& parent
		)
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
	}

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
		default:
			break;
		}
	}

	void ComputedStyle::Apply(const StyleEntry& e, ComputedStyle& parent)
	{
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
			mc_textProperties mc_layoutProperties mc_renderProperties default
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
}
