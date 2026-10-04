#pragma once
#include "Identifiers.h"
#include <memory>

namespace mocca
{
	class Application;
	class Surface;
	struct SurfaceDesc;

	namespace detail
	{
		class HookStore;
		class ComponentScope;
		struct Node;
	}

	struct Context
	{
	public:
		Context();
		~Context();

		Context(const Context&) = delete;
		auto operator=(const Context&) -> Context& = delete;

		auto CreateSurface(const SurfaceDesc& desc) -> Surface*;

	private:
		std::unique_ptr<detail::HookStore> _store;
		Surface* _currentSurface{nullptr};

		[[nodiscard]] auto _beginComponent(detail::NodeId id)
			-> detail::ComponentScope;

		friend class Application;
		friend class Surface;
		friend struct Element;
        friend struct detail::Node;
	};

	auto getCtx() -> Context*;
}