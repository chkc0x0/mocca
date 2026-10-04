#include "core/Node.h"
#include "core/HookStore.h"
#include "text/Measure.h"
#include "yoga/YGNodeLayout.h"
#include <iostream>
#include <ranges>
#include "core/Context.h"

namespace mocca::detail
{
	Node::~Node()
	{
		if (YogaNode != nullptr)
		{
			YGNodeFree(YogaNode);
		}
	}

	void Node::AddComponentToYoga(YGNodeRef parent, Node& node)
	{
		for (auto& grandchild : node.Children)
		{
			if (grandchild->IsComponent())
			{
				AddComponentToYoga(parent, *grandchild);
			}
			else
			{
				grandchild->BuildYogaTree();
				YGNodeInsertChild(
					parent,
					grandchild->YogaNode,
					YGNodeGetChildCount(parent)
				);
			}
		}
	}

	void Node::BuildYogaTree()
	{
		if (YogaNode == nullptr)
		{
			return;
		}

		ApplyLayoutStyles();

		YGNodeRemoveAllChildren(YogaNode);

		for (auto& child : Children)
		{
			if (!child)
			{
				continue;
			}

			if (child->IsComponent())
			{
				AddComponentToYoga(YogaNode, *child);
			}
			else
			{
				child->BuildYogaTree();
				YGNodeInsertChild(
					YogaNode,
					child->YogaNode,
					YGNodeGetChildCount(YogaNode)
				);
			}
		}
	}

	void Node::ApplyLayoutStyles() const
	{
#define mc_styleProperty(name, ...)                                            \
	styles::detail::applying::layout::apply##name(YogaNode, Style.name);
		mc_layoutProperties
#undef mc_styleProperty
	}

	void Node::ComputeStyle(const ComputedStyle& parentComputed)
	{
		Style = ComputedStyle::Cascade(Declared, parentComputed);

		for (auto& child : Children)
		{
			if (child)
			{
				child->ComputeStyle(Style);
			}
		}
	}

	auto Node::BuildNodeTree(const Element& element) -> std::unique_ptr<Node>
	{
		auto node = std::make_unique<Node>();
		node->Id = nextNodeId++;
		node->Key = element.Key;
		node->Declared = element.Style;

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

		node->Events = element.Events;

		if (!node->IsComponent())
		{
			node->YogaNode = YGNodeNew();
			YGNodeSetContext(node->YogaNode, node.get());
		}

		return node;
	}

	auto Node::GetX() const -> float
	{
		float pos = 0.0F;
		if (YogaNode != nullptr)
		{
			pos = YGNodeLayoutGetLeft(YogaNode);
		}
		if (Parent != nullptr)
		{
			pos += Parent->GetX();
		}
		return pos;
	}

	auto Node::GetY() const -> float
	{
		float pos = 0.0F;
		if (YogaNode != nullptr)
		{
			pos = YGNodeLayoutGetTop(YogaNode);
		}
		if (Parent != nullptr)
		{
			pos += Parent->GetY();
		}
		return pos;
	}

	void Node::Paint(Canvas& canvas)
	{
		if (IsComponent())
		{
			for (auto& child : Children)
			{
				child->Paint(canvas);
			}
			return;
		}

		float x = GetX();
		float y = GetY();
		float w = YGNodeLayoutGetWidth(YogaNode);
		float h = YGNodeLayoutGetHeight(YogaNode);

		if (IsBox())
		{
			bool clips = Style.Overflow == OverflowType::Hidden ||
						 Style.Overflow == OverflowType::Scroll;
			bool scrolls = Style.Overflow == OverflowType::Scroll;

			if (clips)
			{
				canvas.PushClip(x, y, w, h);
			}

			canvas.DrawRect(x, y, w, h, Style.BackgroundColor);

			if (scrolls)
			{
				canvas.PushTransform(-ScrollOffset.X, -ScrollOffset.Y);
			}

			for (auto& child : Children)
			{
				child->Paint(canvas);
			}

			if (scrolls)
			{
				canvas.PopTransform();
			}

			if (clips)
			{
				canvas.PopClip();
			}
		}
		else if (IsText())
		{
			canvas.DrawText(
				x,
				y,
				std::get<TextElement>(Kind).Content,
				Style.TextColor
			);
		}
		else
		{
			for (auto& children : Children)
			{
				children->Paint(canvas);
			}
		}
	}

	void Node::Print(int depth) const
	{
		std::string indent(static_cast<size_t>(depth * 2), ' ');
		auto kind = NodeKind();

		std::cout << indent << "<" << kind << " id=\"" << Id << "\"";

		if (!std::holds_alternative<ComponentElement>(Kind))
		{
			std::cout << " x=" << GetX() << " y=" << GetY()
					  << " w=" << YGNodeLayoutGetWidth(YogaNode)
					  << " h=" << YGNodeLayoutGetHeight(YogaNode);
		}

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
		for (auto& [K, n] : oldKeyed)
		{
			if (n)
			{
				getCtx()->_store->RemoveComponent(n->Id);
			}
		}
		for (size_t i = unkeyedCursor; i < oldUnkeyed.size(); i++)
		{
			getCtx()->_store->RemoveComponent(oldUnkeyed[i]->Id);
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
			getCtx()->_store->RemoveComponent(oldNode->Id);
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
			// TODO zombie later on
			getCtx()->_store->RemoveComponent(oldNode->Id);
			oldNode = BuildNodeTree(*newElement);
		}

		if (oldNode->IsText())
		{
			const auto& incoming = std::get<TextElement>(newElement->Node);
			bool contentChanged =
				std::get<TextElement>(oldNode->Kind).Content !=
				incoming.Content;

			if (contentChanged)
			{
				oldNode->Kind = incoming;
				YGNodeMarkDirty(oldNode->YogaNode);
			}

			YGNodeSetMeasureFunc(
				oldNode->YogaNode,
				&mocca::mtext::detail::measureFunc
			);
		}

		oldNode->Events = newElement->Events;
		oldNode->Declared = newElement->Style;

		auto childElements = CollectChildElements(newElement, oldNode->Id);
		oldNode->Children = ReconcileChildren(
			std::move(oldNode->Children),
			childElements,
			oldNode.get()
		);

		return oldNode;
	}

	auto layoutRect(const Node& n) -> Rectangle
	{
		return {
			.X = n.GetX(),
			.Y = n.GetY(),
			.Width = YGNodeLayoutGetWidth(n.YogaNode),
			.Height = YGNodeLayoutGetHeight(n.YogaNode),
		};
	}

	auto clipsChildren(const Node& n) -> bool
	{
		return n.IsBox() && n.Style.Overflow != OverflowType::Visible;
	}

	auto hitTestImpl(Node* node, float x, float y, Vector2 scroll) -> Node*
	{
		if (node == nullptr)
		{
			return nullptr;
		}

		bool inside = !node->IsComponent() &&
					  layoutRect(*node).Contains(
						  {.X = x + scroll.X, .Y = y + scroll.Y}
					  );

		if (!inside && clipsChildren(*node))
		{
			return nullptr;
		}

		Vector2 childScroll = scroll;
		if (node->IsBox() && node->Style.Overflow == OverflowType::Scroll)
		{
			childScroll.X += node->ScrollOffset.X;
			childScroll.Y += node->ScrollOffset.Y;
		}

		for (auto& it : std::views::reverse(node->Children))
		{
			if (Node* hit = hitTestImpl(it.get(), x, y, childScroll))
			{
				return hit;
			}
		}

		return inside ? node : nullptr;
	}

	auto Node::HitTest(Node* root, float x, float y) -> Node*
	{
		return hitTestImpl(root, x, y, {});
	}

	auto Node::FindNodeById(Node* root, NodeId id) -> Node*
	{
		if (root == nullptr)
		{
			return nullptr;
		}
		if (root->Id == id)
		{
			return root;
		}
		for (const auto& child : root->Children)
		{
			auto* found = FindNodeById(child.get(), id);
			if (found != nullptr)
			{
				return found;
			}
		}
		return nullptr;
	}
}