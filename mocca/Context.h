#pragma once
#include "Detail.h"

namespace mocca
{
	struct Context
	{
	public:
	private:
		detail::StateStore _store;

		detail::NodeId _componentId;
		uint32_t _hookIndex;

		auto _enterComponentRender(detail::NodeId id) -> detail::NodeId
		{
			detail::NodeId prev = _componentId;
			_componentId = id;
			_hookIndex = 0;
			return prev;
		}

		void _exitComponentRender(detail::NodeId prev)
		{
			_componentId = prev;
		}

		friend struct detail::Node;
		friend struct Element;
		friend class Application;

		template <typename T>
		friend auto useState(T initial)
			-> std::pair<T&, std::function<void(T)>>;
	};

	auto getCtx() -> Context*;

	template <typename T>
	auto useState(T initial) -> std::pair<T&, std::function<void(T)>>
	{
		Context* ctx = getCtx();
		detail::NodeId id = ctx->_componentId;
		std::uint32_t hook = ctx->_hookIndex++;

		T& value = ctx->_store.GetOrCreate<T>(id, hook, std::move(initial));

		auto setter = [ctx, id, hook](T v) -> auto
		{ ctx->_store.Set<T>(id, hook, std::move(v)); };

		return {value, std::move(setter)};
	}
}