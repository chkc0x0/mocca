#pragma once
#include "Canvas.h"
#include "Element.h"
#include "yoga/Yoga.h"
#include <cstdint>
#include <memory>
#include <unordered_set>
#include <variant>
#include <vector>

namespace mocca::detail
{
	using NodeId = std::uint64_t;

	template <typename T>
	concept EqualityComparable = requires(const T& a, const T& b) {
		{ a == b } -> std::convertible_to<bool>;
	};

	struct Node
	{
		Node();
		~Node();

		NodeId Id;
		ElementKey Key = mc_keyNone;

		Node* Parent = nullptr; // not read yet
		YGNodeRef YogaNode{nullptr};

		DeclaredStyle Declared;
		ComputedStyle Style;

		std::vector<std::unique_ptr<Node>> Children;

		std::variant<std::monostate, TextElement, ComponentElement> Kind;

		[[nodiscard]] auto IsBox() const -> bool
		{
			return std::holds_alternative<std::monostate>(Kind);
		}
		[[nodiscard]] auto IsText() const -> bool
		{
			return std::holds_alternative<TextElement>(Kind);
		}
		[[nodiscard]] auto IsComponent() const -> bool
		{
			return std::holds_alternative<ComponentElement>(Kind);
		}

		[[nodiscard]] auto NodeKind() const -> std::string_view
		{
			if (IsBox())
			{
				return "Box";
			}
			if (IsText())
			{
				return "Text";
			}
			if (IsComponent())
			{
				return "Component";
			}

			return "Unknown";
		}

		static auto BuildNodeTree(const Element& element)
			-> std::unique_ptr<Node>;
		void BuildYogaTree();
		void ApplyLayoutStyles() const;
		void ComputeStyle(const ComputedStyle& parentComputed);

		[[nodiscard]] auto GetX() const -> float;
		[[nodiscard]] auto GetY() const -> float;

		static auto CollectChildElements(
			const Element* newElement,
			NodeId ownerId
		) -> std::vector<Element>;

		static auto ReconcileChildren(
			std::vector<std::unique_ptr<Node>> oldChildren,
			const std::vector<Element>& childElements,
			Node* parent
		) -> std::vector<std::unique_ptr<Node>>;

		static auto Reconcile(
			std::unique_ptr<Node> oldNode,
			const Element* newElement
		) -> std::unique_ptr<Node>;

		void Print(int depth = 0);
		void Paint(Canvas& canvas);
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
	};

	struct EffectSlot
	{
		std::vector<EffectDependency> LastDeps;
		CleanupFn Cleanup;
		bool HasRun = false;
	};

	class HookStore
	{
	public:
		template <typename T>
		auto GetOrCreate(NodeId id, std::uint32_t hook, T initial) -> T&
		{
			HookKey key{.Id = id, .Hook = hook};
			auto it = _slots.find(key);
			if (it == _slots.end())
			{
				// single insert, keep the returned iterator (no double-lookup)
				it = _slots.emplace(key, std::any(std::move(initial))).first;
			}
			return any_cast<T&>(it->second);
		}

		template <typename T> void Set(NodeId id, std::uint32_t hook, T value)
		{
			auto it = _slots.find(HookKey{.Id = id, .Hook = hook});
			if (it != _slots.end())
			{
				it->second = std::move(value);
				if (_markDirty)
				{
					_markDirty(id);
				}
			}
		}

		auto GetEffectSlot(NodeId id, std::uint32_t hook) -> EffectSlot&
		{
			HookKey key{.Id = id, .Hook = hook};
			auto it = _slots.find(key);
			if (it == _slots.end())
			{
				it = _slots.emplace(key, std::any(EffectSlot{})).first;
			}
			return std::any_cast<EffectSlot&>(it->second);
		}

		void FlushEffects()
		{
			auto q = std::move(_effectQueue);
			_effectQueue.clear();
			for (auto& run : q)
			{
				run();
			}
		}

		void RemoveComponent(NodeId id)
		{
			for (auto it = _slots.begin(); it != _slots.end();)
			{
				if (it->first.Id == id)
				{
					if (auto* eff = std::any_cast<EffectSlot>(&it->second))
					{
						if (eff->Cleanup)
						{
							eff->Cleanup();
						}
					}
					it = _slots.erase(it);
				}
				else
				{
					++it;
				}
			}
		}

		void PushEffect(const CleanupFn& fn)
		{
			_effectQueue.push_back(fn);
		}

		void SetMarkDirty(std::function<void(NodeId)> fn)
		{
			_markDirty = std::move(fn);
		}

		void ClearDirty()
		{
			_dirtySet.clear();
		}

		void InsertDirty(NodeId id)
		{
			_dirtySet.insert(id);
		}

	private:
		std::unordered_map<HookKey, std::any, HookKeyHash> _slots;
		std::vector<std::function<void()>> _effectQueue;

		std::unordered_set<NodeId> _dirtySet;
		std::function<void(NodeId)> _markDirty;
	};
}