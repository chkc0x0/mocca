#include "Detail.h"
#include "Context.h"
#include "Style.h"
#include <cstddef>
#include <iostream>
#include <ranges>

namespace mocca::detail
{
	Node::~Node()
	{
		if (YogaNode != nullptr)
		{
			YGNodeFree(YogaNode);
		}
	}

	auto measureFunc(
		const YGNode* ref,
		float with,
		YGMeasureMode widthMode,
		float height,
		YGMeasureMode heightMode
	) -> YGSize
	{
		auto* self = static_cast<Node*>(YGNodeGetContext(ref));
		if ((self == nullptr) || !self->IsText())
		{
			return YGSize{.width = 0, .height = 0};
		}

		const std::string& content = std::get<TextElement>(self->Kind).Content;

		// TODO: read font size from computed style once styling exists. Hardcoded now.
		constexpr int fontSize = 16;

		auto width = static_cast<float>(content.size() * 8);

		return YGSize{.width = width, .height = static_cast<float>(fontSize)};
	}

	void Node::BuildYogaTree()
	{
		if (YogaNode == nullptr)
		{
			return;
		}

		ApplyLayoutStyles();

		YGNodeRemoveAllChildren(YogaNode);

		uint32_t index = 0;
		for (auto& child : Children)
		{
			if (!child)
			{
				continue;
			}

			if (child->IsComponent())
			{
				for (auto& grandchild : child->Children)
				{
					if (!grandchild)
					{
						continue;
					}
					grandchild->BuildYogaTree();
					YGNodeInsertChild(YogaNode, grandchild->YogaNode, index++);
				}
			}
			else
			{
				child->BuildYogaTree();
				YGNodeInsertChild(YogaNode, child->YogaNode, index++);
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
			canvas
				.DrawText(x, y, std::get<TextElement>(Kind).Content, Style.TextColor);
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
			YGNodeSetMeasureFunc(oldNode->YogaNode, &measureFunc);
			YGNodeMarkDirty(oldNode->YogaNode);
		}

		oldNode->Events = newElement->Events;

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

	auto ComputedStyle::Cascade(
		const DeclaredStyle& declared,
		const ComputedStyle& parent
	) -> ComputedStyle
	{
		ComputedStyle computed;
#define mc_styleProperty(name, ...)                                            \
	computed.name = declared.name.Resolve(                                     \
		parent.name,                                                           \
		styles::detail::properties::name##Property                             \
	);
		mc_layoutProperties mc_renderProperties
#undef mcStyleProperty
			return computed;
	}

	namespace detail
	{

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

		auto hitTestImpl(
			Node* root,
			float x,
			float y,
			float accScrollX,
			float accScrollY
		) -> Node*
		{
			if (root == nullptr)
			{
				return nullptr;
			}

			float childAccX = accScrollX;
			float childAccY = accScrollY;
			if (root->IsBox() && root->Style.Overflow == OverflowType::Scroll)
			{
				childAccX += root->ScrollOffset.X;
				childAccY += root->ScrollOffset.Y;
			}

			for (auto& it : std::views::reverse(root->Children))
			{
				auto* hit = hitTestImpl(it.get(), x, y, childAccX, childAccY);
				if (hit != nullptr)
				{
					return hit;
				}
			}

			if (root->IsComponent())
			{
				return nullptr;
			}

			float nx = root->GetX();
			float ny = root->GetY();
			float nw = YGNodeLayoutGetWidth(root->YogaNode);
			float nh = YGNodeLayoutGetHeight(root->YogaNode);

			float testX = x + accScrollX;
			float testY = y + accScrollY;

			if (testX >= nx && testX < nx + nw && testY >= ny &&
				testY < ny + nh)
			{
				return root;
			}

			return nullptr;
		}

		auto Node::HitTest(Node* root, float x, float y) -> Node*
		{
			return hitTestImpl(root, x, y, 0, 0);
		}

		auto findScrollableAncestor(Node* node) -> Node*
		{
			for (Node* n = node; n != nullptr; n = n->Parent)
			{
				if (n->IsBox() && n->Style.Overflow == OverflowType::Scroll)
				{
					return n;
				}
			}
			return nullptr;
		}

	}

}