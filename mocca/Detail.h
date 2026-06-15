#pragma once
#include "Element.h"
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_set>
#include <variant>
#include <vector>

namespace mocca::detail
{
	using NodeId = std::uint64_t;

	struct TextNode
	{
		std::string Content;
	};

	struct ComponentNode
	{
		ComponentFn Fn;
	};

	struct Node
	{
		NodeId Id;
		ElementKey Key = mc_keyNone;

		Node* Parent = nullptr; // not read yet

		std::vector<std::unique_ptr<Node>> Children;

		std::variant<std::monostate, TextNode, ComponentNode> Kind;

		[[nodiscard]] auto IsBox() const -> bool
		{
			return std::holds_alternative<std::monostate>(Kind);
		}
		[[nodiscard]] auto IsText() const -> bool
		{
			return std::holds_alternative<TextNode>(Kind);
		}
		[[nodiscard]] auto IsComponent() const -> bool
		{
			return std::holds_alternative<ComponentNode>(Kind);
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

		static auto Reconcile(std::unique_ptr<Node> oldNode,
							  const Element* newElement)
			-> std::unique_ptr<Node>;

		void Print(int depth = 0);
	};

	struct StateKey
	{
		NodeId Id;
		std::uint32_t Hook;
		auto operator==(const StateKey&) const -> bool = default;
	};
	struct StateKeyHash
	{
		auto operator()(const StateKey& k) const -> std::size_t
		{
			return std::hash<std::uint64_t>{}(k.Id) ^
				   (std::hash<std::uint32_t>{}(k.Hook) << 1);
		}
	};

	class StateStore
	{
	public:
		template <typename T>
		auto GetOrCreate(NodeId id, std::uint32_t hook, T initial) -> T&
		{
			StateKey key{.Id = id, .Hook = hook};
			auto it = _slots.find(key);
			if (it == _slots.end())
			{
				// single insert, keep the returned iterator (no double-lookup)
				it =
					_slots.emplace(key, std::make_shared<T>(std::move(initial)))
						.first;
			}
			return *static_cast<T*>(it->second.get());
		}

		template <typename T> void Set(NodeId id, std::uint32_t hook, T value)
		{
			auto it = _slots.find(StateKey{.Id = id, .Hook = hook});
			if (it != _slots.end())
			{
				*static_cast<T*>(it->second.get()) = std::move(value);
				if (_markDirty)
				{
					_markDirty(id);
				}
			}
		}

		void RemoveComponent(NodeId id)
		{
			// called on node removal/replace (the clang-flagged obligation)
			for (auto it = _slots.begin(); it != _slots.end();)
			{
				it = (it->first.Id == id) ? _slots.erase(it) : std::next(it);
			}
		}

		void SetMarkDirty(std::function<void(NodeId)> fn)
		{
			_markDirty = std::move(fn);
		}

	private:
		std::unordered_map<StateKey, std::shared_ptr<void>, StateKeyHash>
			_slots;
		std::unordered_set<NodeId> _dirtySet;
		std::function<void(NodeId)> _markDirty = [this](NodeId id) -> void
		{ _dirtySet.insert(id); };
	};
}