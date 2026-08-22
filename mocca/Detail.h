#pragma once
#include "Canvas.h"
#include "Element.h"
#include "InputTypes.h"
#include "yoga/Yoga.h"
#include <cstdint>
#include <memory>
#include <unordered_set>
#include <variant>
#include <vector>

namespace mocca::detail
{
	using NodeId = std::uint64_t;

	inline NodeId nextNodeId = 0;

	template <typename T>
	concept EqualityComparable = requires(const T& a, const T& b) {
		{ a == b } -> std::convertible_to<bool>;
	};

	constexpr auto hashString(std::string_view str) -> uint64_t
	{
		uint64_t hash = 14695981039346656037ULL;
		for (char c : str)
		{
			hash ^= static_cast<uint8_t>(c);
			hash *= 1099511628211ULL;
		}
		return hash;
	}

	inline auto toUtf8(const std::u32string& input) -> std::string
	{
		std::string result;
		for (char32_t cp : input)
		{
			if (cp >= 0xD800 && cp <= 0xDFFF)
			{
				continue;
			}
			if (cp > 0x10FFFF)
			{
				continue;
			}

			if (cp < 0x80)
			{
				result.push_back(static_cast<char>(cp));
			}
			else if (cp < 0x800)
			{
				result.push_back(static_cast<char>(0xC0 | (cp >> 6)));
				result.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
			}
			else if (cp < 0x10000)
			{
				result.push_back(static_cast<char>(0xE0 | (cp >> 12)));
				result.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
				result.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
			}
			else
			{
				result.push_back(static_cast<char>(0xF0 | (cp >> 18)));
				result.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
				result.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
				result.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
			}
		}
		return result;
	}

	inline auto toUtf8(char32_t cp) -> std::string
	{
		return toUtf8(std::u32string{cp});
	}

	struct Node
	{
		Node() = default;
		~Node();

		NodeId Id;
		ElementKey Key = mc_keyNone;

		Node* Parent = nullptr; // not read yet
		YGNodeRef YogaNode{nullptr};

		DeclaredStyle Declared;
		ComputedStyle Style;

		Vector2 ScrollOffset{.X=0, .Y=0};

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

		void Print(int depth = 0) const;
		void Paint(Canvas& canvas);

		static auto HitTest(Node* root, float x, float y) -> Node*;
		static auto FindNodeById(Node* root, NodeId id) -> Node*;
		static void AddComponentToYoga(YGNodeRef parent, Node& node);

		EventHandlers Events;
	};

	auto findScrollableAncestor(Node* node) -> Node*;

	template <typename Fn>
	auto dispatchPointerEvent(
		Node* root,
		NodeId capturedNode,
		PointerEvent& ev,
		Fn callback
	) -> Node*
	{
		Node* target = nullptr;
		if (capturedNode != 0)
		{
			target = Node::FindNodeById(root, capturedNode);
		}
		if (target == nullptr)
		{
			target = Node::HitTest(root, ev.X, ev.Y);
		}
		if (target == nullptr)
		{
			return nullptr;
		}
		std::vector<Node*> path;
		for (Node* n = target; n != nullptr; n = n->Parent)
		{
			path.push_back(n);
		}
		for (Node* n : path)
		{
			if (ev.StopPropagation)
			{
				break;
			}
			callback(n, ev);
		}

		return target;
	}

	template <typename Fn>
	void
	dispatchKeyEvent(Node* root, NodeId focusedNode, KeyEvent& ev, Fn callback)
	{
		Node* target = nullptr;
		if (focusedNode != 0)
		{
			target = Node::FindNodeById(root, focusedNode);
		}
		if (target == nullptr)
		{
			return;
		}
		std::vector<Node*> path;
		for (Node* n = target; n != nullptr; n = n->Parent)
		{
			path.push_back(n);
		}
		for (Node* n : path)
		{
			if (ev.StopPropagation)
			{
				break;
			}
			callback(n, ev);
		}
	}

	template <typename Fn>
	void dispatchTextEvent(
		Node* root,
		NodeId focusedNode,
		TextEvent& ev,
		Fn callback
	)
	{
		Node* target = nullptr;
		if (focusedNode != 0)
		{
			target = Node::FindNodeById(root, focusedNode);
		}
		if (target == nullptr)
		{
			return;
		}
		std::vector<Node*> path;
		for (Node* n = target; n != nullptr; n = n->Parent)
		{
			path.push_back(n);
		}
		for (Node* n : path)
		{
			if (ev.StopPropagation)
			{
				break;
			}
			callback(n, ev);
		}
	}

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
			if (it != _slots.end())
			{
				mc_error(
					ErrorCode::InvalidState,
					"hook already exists; Create called twice. this will "
					"overwrite data"
				);
			}
			it = _slots.emplace(key, std::any(std::move(initial))).first;
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
			else
			{
				mc_error(ErrorCode::InvalidArgument, "non existent hook slot");
			}
		}

		auto Has(NodeId id, std::uint32_t hook) -> bool
		{
			HookKey key{.Id = id, .Hook = hook};
			return _slots.contains(key);
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

		auto GetMemoSlot(NodeId id, std::uint32_t hook) -> MemoSlot&
		{
			HookKey key{.Id = id, .Hook = hook};
			auto it = _slots.find(key);
			if (it == _slots.end())
			{
				it = _slots.emplace(key, std::any(MemoSlot{})).first;
			}
			return std::any_cast<MemoSlot&>(it->second);
		}

		void FlushEffects()
		{
			auto q = std::move(_effectQueue);
			_effectQueue.clear();
			for (auto& run : q)
			{
				try
				{
					run();
				}
				catch (const std::exception& e)
				{
					mc_error(
						ErrorCode::UserSide,
						"effect threw an exception: {}",
						e.what()
					);
				}
				catch (...)
				{
					mc_error(
						ErrorCode::UserSide,
						"effect threw a non-std exception"
					);
				}
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
						auto old = std::move(eff->Cleanup);
						eff->Cleanup = nullptr;
						if (old)
						{
							try
							{
								old();
							}
							catch (const std::exception& e)
							{
								mc_error(
									ErrorCode::UserSide,
									"cleanup threw during removal: {}",
									e.what()
								);
							}
							catch (...)
							{
								mc_error(
									ErrorCode::UserSide,
									"cleanup threw a non-std exception during "
									"removal"
								);
							}
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