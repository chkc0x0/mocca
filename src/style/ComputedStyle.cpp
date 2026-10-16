#include "style/ComputedStyle.h"
#include "core/Logger.h"
#include "anim/Interpolate.h"

namespace mocca::styles
{
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
			mc_layoutProperties mc_renderProperties default
				: mc_error(
					  ErrorCode::InvalidArgument,
					  "how did we get to this style? ({})", (int)e.Prop
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

		return out;
	}

	auto getDefaultStyle() -> ComputedStyle&
	{
		// TODO: make this interned so it's just a bunch of handles instead of this mess
		static ComputedStyle style;
		return style;
	}
}
