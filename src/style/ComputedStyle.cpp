#include "style/ComputedStyle.h"
#include "core/Logger.h"

namespace mocca::styles
{
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
