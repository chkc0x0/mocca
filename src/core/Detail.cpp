#include "Detail.h"
#include "Context.h"
#include <cstddef>
#include <iostream>

namespace mocca::detail
{
	static NodeId nextNodeId = 0;

	Node::Node()
	{
		YogaNode = YGNodeNew();
	}

	Node::~Node()
	{
		YGNodeFree(YogaNode);
	}

	void Node::BuildYogaTree()
	{
		ApplyLayoutStyles();
		YGNodeRemoveAllChildren(YogaNode);

		uint32_t index = 0;
		for (auto& child : Children)
		{
			if (!child)
			{
				continue;
			}
			child->BuildYogaTree();
			YGNodeInsertChild(YogaNode, child->YogaNode, index++);
		}
	}

	void Node::ApplyLayoutStyles()
	{
		const auto& s = Style;

		if (s.Width.IsValue())
		{
			YGNodeStyleSetWidth(YogaNode, s.Width.GetValue().Value);
		}
		if (s.Height.IsValue())
		{
			YGNodeStyleSetHeight(YogaNode, s.Height.GetValue().Value);
		}
	}

	auto Node::BuildNodeTree(const Element& element) -> std::unique_ptr<Node>
	{
		auto node = std::make_unique<Node>();
		node->Id = nextNodeId++;
		node->Key = element.Key;
		node->Style = element.Style;

		std::visit(
			[&](const auto& arm) -> auto
			{
				using T = std::decay_t<decltype(arm)>;
				if constexpr (std::is_same_v<T, BoxElement>)
				{
					node->Kind = std::monostate{};
					for (const auto& child : arm.Children)
					{
						auto childNode = BuildNodeTree(child);
						childNode->Parent = node.get();
						node->Children.push_back(std::move(childNode));
					}
				}
				else if constexpr (std::is_same_v<T, TextElement>)
				{
					node->Kind = TextElement{.Content = arm.Content};
				}
				else if constexpr (std::is_same_v<T, ComponentElement>)
				{
					node->Kind = ComponentElement{.Fn = arm.Fn};
				}
			},
			element.Node
		);

		return node;
	}

	auto Node::GetX() const -> float
	{
		return YGNodeLayoutGetLeft(YogaNode) +
			   (Parent == nullptr ? 0 : Parent->GetX());
	}

	auto Node::GetY() const -> float
	{
		return YGNodeLayoutGetTop(YogaNode) +
			   (Parent == nullptr ? 0 : Parent->GetY());
	}

	void Node::Paint(Canvas& canvas)
	{
		float x = GetX();
		float y = GetY();
		float w = YGNodeLayoutGetWidth(YogaNode);
		float h = YGNodeLayoutGetHeight(YogaNode);

		if (IsBox())
		{
			canvas.DrawRect(x, y, w, h, {.R = 200, .G = 200, .B = 200});
		}

		if (IsText())
		{
			canvas.DrawText(
				x,
				y,
				std::get<TextElement>(Kind).Content,
				{.R = 0, .G = 0, .B = 0}
			);
		}

		for (auto& children : Children)
		{
			children->Paint(canvas);
		}
	}

	void Node::Print(int depth)
	{
		std::string indent(static_cast<size_t>(depth * 2), ' ');
		auto kind = NodeKind();

		std::cout << indent << "<" << kind << " id=\"" << Id << "\"";

		if (Key != mc_keyNone)
		{
			std::cout << " key=\"" << Key << "\"";
		}

		std::cout << ((Children.empty() && kind != "Text") ? "/>" : ">")
				  << '\n';

		if (IsText())
		{
			const auto& textNode = std::get<TextElement>(Kind);
			std::cout << indent << "  " << textNode.Content << '\n';
		}

		for (const auto& child : Children)
		{
			child->Print(depth + 1);
		}

		if (kind == "Text" || !Children.empty())
		{
			std::cout << indent << "</" << kind << ">" << '\n';
		}
	}

	auto Node::CollectChildElements(const Element* newElement, NodeId ownerId)
		-> std::vector<Element>
	{
		std::vector<Element> childElements;

		if (std::holds_alternative<BoxElement>(newElement->Node))
		{
			childElements = std::get<BoxElement>(newElement->Node).Children;
		}
		else if (std::holds_alternative<ComponentElement>(newElement->Node))
		{
			const auto& component = std::get<ComponentElement>(
				newElement->Node
			);
			auto produced =
				Element::Render(component.Fn, component.Props, ownerId);
			childElements.push_back(std::move(produced));
		}

		return childElements;
	}

	auto Node::ReconcileChildren(
		std::vector<std::unique_ptr<Node>> oldChildren,
		const std::vector<Element>& childElements,
		Node* parent
	) -> std::vector<std::unique_ptr<Node>>
	{
		std::unordered_map<ElementKey, std::unique_ptr<Node>> oldKeyed;
		std::vector<std::unique_ptr<Node>> oldUnkeyed;
		for (auto& child : oldChildren)
		{
			if (child && child->Key != mc_keyNone)
			{
				oldKeyed.emplace(child->Key, std::move(child));
			}
			else if (child)
			{
				oldUnkeyed.push_back(std::move(child));
			}
		}

		std::vector<std::unique_ptr<Node>> newChildren;
		size_t unkeyedCursor = 0;
		for (const auto& child : childElements)
		{
			std::unique_ptr<Node> matchedOld;
			if (child.Key != mc_keyNone)
			{
				auto it = oldKeyed.find(child.Key);
				if (it != oldKeyed.end())
				{
					matchedOld = std::move(it->second);
					oldKeyed.erase(it);
				}
			}
			else if (unkeyedCursor < oldUnkeyed.size())
			{
				matchedOld = std::move(oldUnkeyed[unkeyedCursor++]);
			}

			auto reconciled = Reconcile(std::move(matchedOld), &child);
			if (reconciled)
			{
				reconciled->Parent = parent;
				newChildren.push_back(std::move(reconciled));
			}
		}

		// kill orphans
		for (auto& [k, n] : oldKeyed)
		{
			if (n)
			{
				getCtx()->_store.RemoveComponent(n->Id);
			}
		}
		for (size_t i = unkeyedCursor; i < oldUnkeyed.size(); i++)
		{
			getCtx()->_store.RemoveComponent(oldUnkeyed[i]->Id);
		}

		return newChildren;
	}

	auto Node::Reconcile(
		std::unique_ptr<Node> oldNode,
		const Element* newElement
	) -> std::unique_ptr<Node>
	{
		if (newElement == nullptr && oldNode == nullptr)
		{
			return nullptr;
		}
		if (newElement == nullptr) // oldNode != nullptr implied
		{
			getCtx()->_store.RemoveComponent(oldNode->Id);
			return nullptr;
		}

		if (oldNode == nullptr)
		{
			oldNode = BuildNodeTree(*newElement);
		}
		else if (
			oldNode->Kind.index() != newElement->Node.index() ||
			oldNode->Key != newElement->Key
		)
		{
			// TODO(zombie): later
			getCtx()->_store.RemoveComponent(oldNode->Id);
			oldNode = BuildNodeTree(*newElement);
		}

		if (oldNode->IsText())
		{
			oldNode->Kind = std::get<TextElement>(newElement->Node);
		}

		auto childElements = CollectChildElements(newElement, oldNode->Id);
		oldNode->Children = ReconcileChildren(
			std::move(oldNode->Children),
			childElements,
			oldNode.get()
		);

		return oldNode;
	}
}

namespace mocca
{
	// for the surface
	auto Element::Render(const ComponentFn& fn, std::uint64_t id) -> Element
	{
		auto prev = getCtx()->_enterComponentRender(id);
		auto produced = fn();
		getCtx()->_exitComponentRender(prev);
		return produced;
	}

	// anywhere else
	auto Element::Render(
		const ComponentPropsFn& fn,
		const std::any& props,
		std::uint64_t id
	) -> Element
	{
		auto prev = getCtx()->_enterComponentRender(id);
		auto produced = fn(props);
		getCtx()->_exitComponentRender(prev);
		return produced;
	}
}