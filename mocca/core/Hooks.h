#pragma once
#include "HookStore.h"
#include "HookFrame.h"

namespace mocca
{
	template <typename T> struct StateSetter
	{
	private:
		detail::HookStore* _store;
		detail::NodeId _id;
		std::uint32_t _hook;

	public:
		StateSetter(
			detail::HookStore* store,
			detail::NodeId id,
			std::uint32_t hook
		)
			: _store(store), _id(id), _hook(hook) {};

		void operator()(T v) const
		{
			_store->Set<T>(_id, _hook, std::move(v));
		}

		void operator()(const std::function<T(const T&)>& fn) const
		{
			T& current = _store->Get<T>(_id, _hook);
			_store->Set<T>(_id, _hook, fn(current));
		}
	};

	template <typename T, typename U> struct StateDispatch
	{
	private:
		detail::HookStore* _store;
		detail::NodeId _id;
		std::uint32_t _hook;
		std::function<T(const T&, const U&)> _reducer;

	public:
		StateDispatch(
			detail::HookStore* store,
			detail::NodeId id,
			std::uint32_t hook,
			std::function<T(const T&, const U&)> reducer
		)
			: _store(store), _id(id), _hook(hook),
			  _reducer(std::move(reducer)) {};

		void operator()(U v) const
		{
			T& cur = _store->Get<T>(_id, _hook);
			_store->Set<T>(_id, _hook, _reducer(cur, v));
		}
	};

	namespace detail
	{
		template <typename Slot>
		auto depsChanged(Slot& slot, const std::vector<EffectDependency>& deps)
			-> bool
		{
			bool changed = !slot.HasRun || slot.LastDeps != deps;
			if (changed)
			{
				slot.LastDeps = deps;
			}
			return changed;
		}

		template <typename Fn>
		void
		runEffect(HookStore& store, NodeId id, std::uint32_t hook, Fn effect)
		{
			EffectSlot& s = store.GetEffectSlot(id, hook);

			auto old = std::move(s.Cleanup);
			s.Cleanup = nullptr;
			if (old)
			{
				old();
			}

			if constexpr (std::is_void_v<std::invoke_result_t<Fn>>)
			{
				effect();
			}
			else
			{
				s.Cleanup = effect();
			}
			s.HasRun = true;
		}

		template <bool Layout, typename Fn>
		void pushEffect(const HookFrame& f, std::uint32_t hook, Fn effect)
		{
			HookStore* store = f.Store;
			NodeId id = f.Component;

			auto cb = [store, id, hook, effect]() -> auto
			{ runEffect(*store, id, hook, effect); };

			if constexpr (Layout)
			{
				store->PushLayoutEffect(cb, f.CurrentSurface);
			}
			else
			{
				store->PushEffect(cb);
			}
		}
	}

	template <typename T>
	auto useState(T initial) -> std::pair<T&, StateSetter<T>>
	{
		auto& f = detail::currentFrame();
		std::uint32_t hook = f.NextHook++;

		T& value =
			f.Store->GetOrCreate<T>(f.Component, hook, std::move(initial));

		return {value, StateSetter<T>{f.Store, f.Component, hook}};
	}

	template <typename T, typename U>
	auto useReducer(
		std::function<T(const T&, const U&)> reducer,
		T initialArg,
		std::function<T(const T&)> init = nullptr
	) -> std::pair<T&, StateDispatch<T, U>>
	{
		auto& f = detail::currentFrame();
		std::uint32_t hook = f.NextHook++;
		detail::HookStore& store = *f.Store;

		T& value = store.Has(f.Component, hook)
					   ? store.Get<T>(f.Component, hook)
					   : store.Create<T>(
							 f.Component,
							 hook,
							 init ? init(initialArg) : initialArg
						 );

		return {
			value,
			StateDispatch<T, U>{&store, f.Component, hook, std::move(reducer)}
		};
	}

	template <typename T> auto useRef(T initial) -> T&
	{
		auto& f = detail::currentFrame();
		std::uint32_t hook = f.NextHook++;

		return f.Store->GetOrCreate<T>(f.Component, hook, std::move(initial));
	}

	template <typename T> auto useRef() -> T&
	{
		auto& f = detail::currentFrame();
		std::uint32_t hook = f.NextHook++;

		return f.Store->GetOrCreate<T>(f.Component, hook, T{});
	}

	template <typename... Args>
	auto deps(Args&&... args) -> std::vector<detail::EffectDependency>
	{
		return std::vector<detail::EffectDependency>{
			detail::EffectDependency::Make(std::forward<Args>(args))...
		};
	}

	template <typename Fn> void useEffect(Fn effect)
	{
		auto& f = detail::currentFrame();
		std::uint32_t hook = f.NextHook++;

		detail::pushEffect<false>(f, hook, std::move(effect));
	}

	template <typename Fn>
	void useEffect(Fn effect, const std::vector<detail::EffectDependency>& deps)
	{
		auto& f = detail::currentFrame();
		std::uint32_t hook = f.NextHook++;

		auto& slot = f.Store->GetEffectSlot(f.Component, hook);
		if (detail::depsChanged(slot, deps))
		{
			detail::pushEffect<false>(f, hook, std::move(effect));
		}
	}

	template <typename Fn> void useLayoutEffect(Fn effect)
	{
		auto& f = detail::currentFrame();
		std::uint32_t hook = f.NextHook++;

		detail::pushEffect<true>(f, hook, std::move(effect));
	}

	template <typename Fn>
	void useLayoutEffect(
		Fn effect,
		const std::vector<detail::EffectDependency>& deps
	)
	{
		auto& f = detail::currentFrame();
		std::uint32_t hook = f.NextHook++;

		auto& slot = f.Store->GetEffectSlot(f.Component, hook);
		if (detail::depsChanged(slot, deps))
		{
			detail::pushEffect<true>(f, hook, std::move(effect));
		}
	}

	template <typename T>
	auto useMemo(
		std::function<T()> cb,
		const std::vector<detail::EffectDependency>& deps
	) -> const T&
	{
		auto& f = detail::currentFrame();
		std::uint32_t hook = f.NextHook++;

		detail::MemoSlot& slot = f.Store->GetMemoSlot(f.Component, hook);
		if (detail::depsChanged(slot, deps))
		{
			slot.Value = cb();
			slot.HasRun = true;
		}

		return std::any_cast<const T&>(slot.Value);
	}

	template <typename Fn>
	auto useCallback(Fn fn, const std::vector<detail::EffectDependency>& deps)
		-> const Fn&
	{
		return useMemo<Fn>(std::function<Fn()>{[fn] { return fn; }}, deps);
	}
}