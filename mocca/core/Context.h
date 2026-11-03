#pragma once
#include "Identifiers.h"
#include <memory>
#include <unordered_map>

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
		auto FindNodeById(detail::NodeId id) -> detail::Node*;

		[[nodiscard]] auto TimeMs() const -> double
		{
			return _timeMs;
		}

		void RequestRepaint();

	private:
		std::unique_ptr<detail::HookStore> _store;
		std::unordered_map<detail::NodeId, detail::Node*> _nodes;
		Surface* _currentSurface{nullptr};
		double _timeMs{0.0};

		[[nodiscard]] auto _beginComponent(detail::NodeId id)
			-> detail::ComponentScope;

		friend class Application;
		friend class Surface;
		friend struct Element;
		friend struct detail::Node;
	};

	auto getCtx() -> Context*;
}