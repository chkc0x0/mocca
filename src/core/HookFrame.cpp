#include "core/HookFrame.h"
#include "core/Logger.h"

namespace mocca::detail
{
	namespace
	{
		thread_local HookFrame* current = nullptr;
	}

	auto hasCurrentFrame() -> bool
	{
		return current != nullptr;
	}

	auto currentFrame() -> HookFrame&
	{
		mc_assert(
			current != nullptr,
			"hooks can only be called while a component is rendering"
		);
		return *current;
	}

	ComponentScope::ComponentScope(
		HookStore& store,
		Surface* surface,
		NodeId id
	)
		: _frame{
			  .Store = &store,
			  .CurrentSurface = surface,
			  .Component = id,
			  .NextHook = 0
		  },
		  _prev(current)
	{
		current = &_frame;
	}

	ComponentScope::~ComponentScope()
	{
		current = _prev;
	}
}