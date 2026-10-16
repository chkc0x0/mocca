#pragma once
#include "Identifiers.h"
#include "core/Logger.h"
#include <any>
#include <type_traits>
#include <functional>
#include <unordered_set>

namespace mocca
{
    class Surface;

	using CleanupFn = std::function<void()>;
	using EffectFn = std::function<CleanupFn()>;
}

namespace mocca::detail
{
	template <typename T>
	concept EqualityComparable = requires(const T& a, const T& b) {
		{ a == b } -> std::convertible_to<bool>;
	};
	
	template <typename T>
	struct StateComparable
		: std::bool_constant<std::is_scalar_v<T> || std::is_enum_v<T>>
	{
	};
	
	struct HookKey
	{
		NodeId Id;
		std::uint32_t Hook;
		auto operator==(const HookKey&) const -> bool = default;
	};

	struct HookKeyHash
	{
		auto operator()(const HookKey& k) const -> std::size_t
		{
			return std::hash<std::uint64_t>{}(k.Id) ^
				   (std::hash<std::uint32_t>{}(k.Hook) << 1);
		}
	};

	struct EffectDependency
	{
		std::any Value;
		std::function<bool(const std::any&)> Equals;

		template <typename T> static auto Make(T v) -> EffectDependency
		{
			EffectDependency d;
			d.Value = v;
			if constexpr (EqualityComparable<T>)
			{
				T captured = v;
				d.Equals = [captured](const std::any& other) -> bool
				{
					const T* o = std::any_cast<T>(&other);
					return o && (captured == *o);
				};
			}
			else
			{
				d.Equals = [](const std::any&) -> auto { return false; };
			}
			return d;
		}

		auto operator==(const std::any& other) const -> bool
		{
			return Equals(other);
		}

		auto operator==(const EffectDependency& other) const -> bool
		{
			return Equals(other.Value);
		}
	};
	
	struct EffectSlot
	{
		std::vector<EffectDependency> LastDeps;
		CleanupFn Cleanup;
		bool HasRun = false;
	};

	struct MemoSlot
	{
		std::any Value;
		std::vector<EffectDependency> LastDeps;
		bool HasRun = false;
	};

	struct EffectFunction
	{
		std::function<void()> Callback;
		bool IsLayout = false;
		Surface* OwningSurface = nullptr;
	};

	using AnimationFn = std::function<bool(double dt)>;

	struct AnimationFunction
	{
		NodeId Id = 0;
		std::uint32_t Hook = 0;
		AnimationFn Callback;
	};

	class HookStore
	{
	public:
		template <typename T>
		auto GetOrCreate(NodeId id, std::uint32_t hook, T initial) -> T&
		{
			if (!Has(id, hook))
			{
				return Create<T>(id, hook, initial);
			}

			return Get<T>(id, hook);
		}

		template <typename T> auto Get(NodeId id, std::uint32_t hook) -> T&
		{
			HookKey key{.Id = id, .Hook = hook};
			auto it = _slots.find(key);
			mc_assert(it != _slots.end(), "expected hook to exist");
			return any_cast<T&>(it->second);
		}

		template <typename T>
		auto Create(NodeId id, std::uint32_t hook, T initial) -> T&
		{
			HookKey key{.Id = id, .Hook = hook};
			auto it = _slots.find(key);

			mc_assert(
				it == _slots.end(),
				"expected hook to not exist prior to call"
			);
			it = _slots.emplace(key, std::any(std::move(initial))).first;
			return any_cast<T&>(it->second);
		}

		template <typename T> void Set(NodeId id, std::uint32_t hook, T value)
		{
			auto it = _slots.find(HookKey{.Id = id, .Hook = hook});

			mc_assert(it != _slots.end(), "expected an existent hook slot");

			if constexpr (StateComparable<T>::value)
			{
				auto& current = any_cast<T&>(it->second);
				if (current == value)
				{
					return;
				}
			}

			it->second = std::move(value);
			if (_markDirty)
			{
				_markDirty(id);
			}
		}

		template <typename T, typename Fn>
		void Mutate(NodeId id, std::uint32_t hook, Fn fn)
		{
			auto it = _slots.find(HookKey{.Id = id, .Hook = hook});
			mc_assert(it != _slots.end(), "expected an existent hook slot");
			fn(any_cast<T&>(it->second));
			if (_markDirty)
			{
				_markDirty(id);
			}
		}

		auto Has(NodeId id, std::uint32_t hook) -> bool;
		auto GetEffectSlot(NodeId id, std::uint32_t hook) -> EffectSlot&;
		auto GetMemoSlot(NodeId id, std::uint32_t hook) -> MemoSlot&;
		void FlushEffects();
		void PushAnimation(NodeId id, std::uint32_t hook, AnimationFn fn);
		auto FlushAnimations(double dt) -> std::size_t;
		[[nodiscard]] auto ActiveAnimationCount() const -> std::size_t;
		void FlushLayoutEffects(Surface* surface);
		void RemoveComponent(NodeId id);
		void PushEffect(const CleanupFn& fn);
		void PushLayoutEffect(const CleanupFn& fn, Surface* surface);
		void SetMarkDirty(std::function<void(NodeId)> fn);
		void ClearDirty();
		void InsertDirty(NodeId id);
		auto DirtyCount() const -> size_t;
		auto LastDirty() const -> uint64_t;

	private:
		std::unordered_map<HookKey, std::any, HookKeyHash> _slots;
		std::vector<EffectFunction> _effectQueue;
		std::vector<AnimationFunction> _animations;

		std::unordered_set<NodeId> _dirtySet;
		std::function<void(NodeId)> _markDirty;

		uint64_t _lastDirty = 0;
	};
}