#pragma once
#include "Detail.h"
#include "Surface.h"

namespace mocca
{
	template <typename T> struct StateSetter;

	struct Context
	{
	public:
		auto CreateSurface(const SurfaceDesc& desc) -> Surface*;

	private:
		detail::HookStore _store;
		Surface* _currentSurface{nullptr};

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
		friend class Surface;

		template <typename T>
		friend auto useState(T initial) -> std::pair<T&, StateSetter<T>>;

		template <typename T> friend struct StateSetter;

		template <typename Fn>
		friend void
		useEffect(Fn effect, const std::vector<detail::EffectDependency>& deps);
		template <typename Fn> friend void useEffect(Fn effect);
	};

	auto getCtx() -> Context*;

	template <typename T> struct StateSetter
	{
	private:
		Context* _ctx;
		detail::NodeId _id;
		std::uint32_t _hook;

	public:
		StateSetter(Context* ctx, detail::NodeId id, std::uint32_t hook)
			: _ctx(ctx), _id(id), _hook(hook) {};

		void operator()(T v) const
		{
			_ctx->_store.Set<T>(_id, _hook, std::move(v));
		}

		void operator()(std::function<T(const T&)> fn) const
		{
			T& current = _ctx->_store.GetOrCreate<T>(_id, _hook, T{});
			_ctx->_store.Set<T>(_id, _hook, fn(current));
		}
	};

	template <typename T>
	auto useState(T initial) -> std::pair<T&, StateSetter<T>>
	{
		Context* ctx = getCtx();
		detail::NodeId id = ctx->_componentId;
		std::uint32_t hook = ctx->_hookIndex++;

		T& value = ctx->_store.GetOrCreate<T>(id, hook, std::move(initial));

		return {value, StateSetter<T>{ctx, id, hook}};
	}

	template <typename Fn> void useEffect(Fn effect)
	{
		Context* ctx = getCtx();
		detail::NodeId id = ctx->_componentId;
		std::uint32_t hook = ctx->_hookIndex++;
		detail::EffectSlot& slot = ctx->_store.GetEffectSlot(id, hook);

		slot.HasRun = true;
		ctx->_store.PushEffect(
			[ctx, id, hook, effect]() -> auto
			{
				detail::EffectSlot& s = ctx->_store.GetEffectSlot(id, hook);
				if (s.Cleanup)
				{
					s.Cleanup();
				}
				if constexpr (std::is_void_v<std::invoke_result_t<Fn>>)
				{
					effect();
					s.Cleanup = nullptr;
				}
				else
				{
					s.Cleanup = effect();
				}
			}
		);
	}

	template <typename... Args>
	auto deps(Args&&... args) -> std::vector<detail::EffectDependency>
	{
		return std::vector<detail::EffectDependency>{
			detail::EffectDependency::Make(std::forward<Args>(args))...
		};
	}

	template <typename Fn>
	void useEffect(Fn effect, const std::vector<detail::EffectDependency>& deps)
	{
		Context* ctx = getCtx();
		detail::NodeId id = ctx->_componentId;
		std::uint32_t hook = ctx->_hookIndex++;
		detail::EffectSlot& slot = ctx->_store.GetEffectSlot(id, hook);

		bool changed = !slot.HasRun || slot.LastDeps != deps;
		slot.LastDeps = deps;
		slot.HasRun = true;
		if (changed)
		{
			ctx->_store.PushEffect(
				[ctx, id, hook, effect]() -> auto
				{
					detail::EffectSlot& s = ctx->_store.GetEffectSlot(id, hook);
					if (s.Cleanup)
					{
						s.Cleanup();
					}
					if constexpr (std::is_void_v<std::invoke_result_t<Fn>>)
					{
						effect();
						s.Cleanup = nullptr;
					}
					else
					{
						s.Cleanup = effect();
					}
				}
			);
		}
	}
}