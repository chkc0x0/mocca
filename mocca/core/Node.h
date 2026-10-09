#pragma once
#include "Element.h"
#include "gfx/Canvas.h"
#include "yoga/YGNode.h"
#include "style/Style.h"
#include <memory>
#include <vector>

namespace mocca::detail
{
	inline NodeId nextNodeId = 1;

	struct Node
	{
		Node() = default;
		~Node();

		NodeId Id;
		ElementKey Key = mc_keyNone;

		Node* Parent = nullptr;
		YGNodeRef YogaNode{nullptr};

		styles::StyleSet Declared;
		styles::ComputedStyle Style;

		Vector2 ScrollOffset{.X = 0, .Y = 0};
		std::vector<std::unique_ptr<Node>> Children;
		std::variant<std::monostate, TextElement, ComponentElement> Kind;
		
		EventHandlers Events;

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

		void SyncYogaChildren();
		static auto BuildNodeTree(const Element& element)
			-> std::unique_ptr<Node>;
		void BuildYogaTree();
		void ApplyLayoutStyles() const;
		void ComputeStyle(styles::ComputedStyle& parentComputed);

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
	};
}