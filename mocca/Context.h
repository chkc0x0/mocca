#pragma once
#include "Detail.h"
#include "Surface.h"

namespace mocca
{
	template <typename T> struct StateSetter;
	template <typename T, typename U> struct StateDispatch;

	template <typename T, typename U>
	auto useReducer(
		std::function<T(const T&, const U&)> reducer,
		T initialArg,
		std::function<T(const T&)> init = nullptr
	) -> std::pair<T&, StateDispatch<T, U>>;

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

		template <typename T, typename U>
		friend auto useReducer(
			std::function<T(const T&, const U&)> reducer,
			T initialArg,
			std::function<T(const T&)> init
		) -> std::pair<T&, StateDispatch<T, U>>;

		template <typename T> friend auto useRef(T initial) -> T&;
		template <typename T> friend auto useRef() -> T&;

		template <typename T> friend struct StateSetter;
		template <typename T, typename U> friend struct StateDispatch;

		template <typename Fn>
		friend void
		useEffect(Fn effect, const std::vector<detail::EffectDependency>& deps);
		template <typename Fn> friend void useEffect(Fn effect);

		template <typename T>
		friend auto useMemo(
			std::function<T()> cb,
			const std::vector<detail::EffectDependency>& deps
		) -> const T&;
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

	template <typename T, typename U> struct StateDispatch
	{
	private:
		Context* _ctx;
		detail::NodeId _id;
		std::uint32_t _hook;
		std::function<T(const T&, const U&)> _reducer;

	public:
		StateDispatch(
			Context* ctx,
			detail::NodeId id,
			std::uint32_t hook,
			std::function<T(const T&, const U&)> reducer
		)
			: _ctx(ctx), _id(id), _hook(hook), _reducer(reducer) {};

		void operator()(U v) const
		{
			T& cur = _ctx->_store.Get<T>(_id, _hook);
			_ctx->_store.Set<T>(_id, _hook, _reducer(cur, v));
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

	template <typename T, typename U>
	auto useReducer(
		std::function<T(const T&, const U&)> reducer,
		T initialArg,
		std::function<T(const T&)> init
	) -> std::pair<T&, StateDispatch<T, U>>
	{
		Context* ctx = getCtx();
		detail::NodeId id = ctx->_componentId;
		std::uint32_t hook = ctx->_hookIndex++;

		T& value = ctx->_store.Has(id, hook)
					   ? ctx->_store.Get<T>(id, hook)
					   : ctx->_store.Create<T>(
							 id,
							 hook,
							 init ? init(initialArg) : initialArg
						 );

		return {value, StateDispatch<T, U>{ctx, id, hook, std::move(reducer)}};
	}

	template <typename T> auto useRef(T initial) -> T&
	{
		Context* ctx = getCtx();
		detail::NodeId id = ctx->_componentId;
		std::uint32_t hook = ctx->_hookIndex++;

		T& value = ctx->_store.GetOrCreate<T>(id, hook, std::move(initial));

		return value;
	}

	template <typename T> auto useRef() -> T&
	{
		Context* ctx = getCtx();
		detail::NodeId id = ctx->_componentId;
		std::uint32_t hook = ctx->_hookIndex++;

		T& value = ctx->_store.GetOrCreate<T>(id, hook, {});

		return value;
	}

	template <typename Fn> void useEffect(Fn effect)
	{
		Context* ctx = getCtx();
		detail::NodeId id = ctx->_componentId;
		std::uint32_t hook = ctx->_hookIndex++;

		ctx->_store.PushEffect(
			[ctx, id, hook, effect]() -> auto
			{
				detail::EffectSlot& s = ctx->_store.GetEffectSlot(id, hook);

				auto old = std::move(s.Cleanup);
				s.Cleanup = nullptr;
				if (old)
				{
					old();
				}

				if constexpr (std::is_void_v<std::invoke_result_t<Fn>>)
				{
					effect();
					s.HasRun = true;
				}
				else
				{
					s.Cleanup = effect();
					s.HasRun = true;
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

		if (changed)
		{
			slot.LastDeps = deps;

			ctx->_store.PushEffect(
				[ctx, id, hook, effect]() -> auto
				{
					detail::EffectSlot& s = ctx->_store.GetEffectSlot(id, hook);

					auto old = std::move(s.Cleanup);
					s.Cleanup = nullptr;
					if (old)
					{
						old();
					}

					if constexpr (std::is_void_v<std::invoke_result_t<Fn>>)
					{
						effect();
						s.HasRun = true;
					}
					else
					{
						s.Cleanup = effect();
						s.HasRun = true;
					}
				}
			);
		}
	}

	template <typename T>
	auto useMemo(
		std::function<T()> cb,
		const std::vector<detail::EffectDependency>& deps
	) -> const T&
	{
		Context* ctx = getCtx();
		detail::NodeId id = ctx->_componentId;
		std::uint32_t hook = ctx->_hookIndex++;
		detail::MemoSlot& slot = ctx->_store.GetMemoSlot(id, hook);

		bool changed = !slot.HasRun || slot.LastDeps != deps;

		if (changed)
		{
			slot.LastDeps = deps;
			slot.Value = cb();
			slot.HasRun = true;
		}

		return std::any_cast<const T&>(slot.Value);
	}

	template <typename Fn>
	auto useCallback(Fn fn, const std::vector<detail::EffectDependency>& deps)
		-> const Fn&
	{
		return useMemo<Fn>(
			std::function<Fn()>{[fn] -> auto { return fn; }},
			deps
		);
	}
}