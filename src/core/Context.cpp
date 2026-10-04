#include "core/Context.h"
#include "core/Application.h"
#include "core/HookStore.h"
#include "core/Surface.h"
#include "core/HookFrame.h"

namespace mocca
{
	Context::Context() : _store(std::make_unique<detail::HookStore>())
	{
	}

	Context::~Context() = default;

	auto Context::_beginComponent(detail::NodeId id) -> detail::ComponentScope
	{
		return detail::ComponentScope{*_store, _currentSurface, id};
	}

	auto Context::CreateSurface(const SurfaceDesc& desc) -> Surface*
	{
		mc_assert(
			_currentSurface != nullptr,
			"CreateSurface cant be called outside component render"
		);

		SurfaceDesc d = desc;
		d.Parent = _currentSurface;
		return Application::main->RegisterSurface(d);
	}
}