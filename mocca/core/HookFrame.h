#pragma once
#include <cstdint>

namespace mocca
{
	class Surface;
}

namespace mocca::detail
{
	class HookStore;

	using NodeId = std::uint64_t;

	struct HookFrame
	{
		HookStore* Store = nullptr;
		Surface* CurrentSurface = nullptr;
		NodeId Component = 0;
		std::uint32_t NextHook = 0;
	};

	auto currentFrame() -> HookFrame&;
	auto hasCurrentFrame() -> bool;

	class ComponentScope
	{
	public:
		ComponentScope(HookStore& store, Surface* surface, NodeId id);
		~ComponentScope();

		ComponentScope(const ComponentScope&) = delete;
		auto operator=(const ComponentScope&) -> ComponentScope& = delete;
		ComponentScope(ComponentScope&&) = delete;
		auto operator=(ComponentScope&&) -> ComponentScope& = delete;

	private:
		HookFrame _frame;
		HookFrame* _prev;
	};
}