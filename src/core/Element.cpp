#include "core/Element.h"
#include "core/Context.h"
#include "core/HookFrame.h"
#include "utils/ScopedProfiler.h"

namespace mocca
{
    // for the surface
	auto Element::Render(const ComponentFn& fn, std::uint64_t id) -> Element
	{
		mc_profileFunction();
		auto scope = getCtx()->_beginComponent(id);
		auto produced = fn();
		return produced;
	}

	// anywhere else
	auto Element::Render(
		const ComponentPropsFn& fn,
		const std::any& props,
		std::uint64_t id
	) -> Element
	{
		auto scope = getCtx()->_beginComponent(id);
		auto produced = fn(props);
		return produced;
	}
}