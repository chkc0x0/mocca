#include "core/Node.h"
#include "core/HookStore.h"
#include "text/Measure.h"
#include "style/StyleApply.h"
#include "yoga/YGNode.h"
#include "yoga/YGNodeLayout.h"
#include <algorithm>
#include <iostream>
#include <ranges>
#include <vector>
#include "core/Context.h"
#include "core/Surface.h"

namespace mocca::detail
{
	Node::~Node()
	{
		if (YogaNode != nullptr)
		{
			YGNodeFree(YogaNode);
		}
	}

	static void CollectFlatYogaChildren(Node& node, std::vector<YGNodeRef>& out)
	{
		for (auto& child : node.Children)
		{
			if (!child)
			{
				continue;
			}

			if (child->IsComponent())
			{
				CollectFlatYogaChildren(*child, out);
			}
			else
			{
				out.push_back(child->YogaNode);
			}
		}
	}

	void Node::SyncYogaChildren()
	{
		if (YogaNode != nullptr)
		{
			ApplyLayoutStyles();

			auto expected = std::vector<YGNodeRef>{};
			CollectFlatYogaChildren(*this, expected);

			size_t have = YGNodeGetChildCount(YogaNode);
			size_t keep = 0;
			while (keep < have && keep < expected.size() &&
				   YGNodeGetChild(YogaNode, keep) == expected[keep])
			{
				++keep;
			}

			while (YGNodeGetChildCount(YogaNode) > keep)
			{
				YGNodeRemoveChild(YogaNode, YGNodeGetChild(YogaNode, keep));
			}

			for (size_t i = keep; i < expected.size(); ++i)
			{
				YGNodeInsertChild(YogaNode, expected[i], i);
			}
		}

		for (auto& child : Children)
		{
			if (child)
			{
				child->SyncYogaChildren();
			}
		}
	}

	void Node::BuildYogaTree()
	{
		SyncYogaChildren();
	}

	void Node::ApplyLayoutStyles() const
	{
#define mc_styleGroup LayoutSizing

#define mc_styleProperty(name, ...)                                            \
	styles::detail::applying::layout::apply##name(                             \
		YogaNode,                                                              \
		Style.mc_styleGroup.name                                                    \
	);
		mc_layoutSizingProps
#undef mc_styleGroup
#define mc_styleGroup LayoutSpacing
			mc_layoutSpacingProps
#undef mc_styleGroup
#define mc_styleGroup LayoutFlex
				mc_layoutFlexProps
#undef mc_styleGroup
#define mc_styleGroup LayoutPosition
					mc_layoutPosProps
#undef mc_styleGroup
#define mc_styleGroup Inherited
			mc_layoutPropertiesInh
#undef mc_styleProperty
	}

	void Node::ComputeStyle(styles::ComputedStyle& parentComputed, double dt)
	{
		styles::ComputedStyle target =
			styles::ComputedStyle::Cascade(Declared, parentComputed);

		if (!Styled)
		{
			Style = target;
			Styled = true;
		}
		else
		{
			AdvanceTransitions(target, dt);
		}

		for (auto& child : Children)
		{
			if (child)
			{
				child->ComputeStyle(Style, dt);
			}
		}
	}

	void Node::AdvanceTransitions(styles::ComputedStyle& target, double dt)
	{
		bool anyActive = false;

#define mc_animatable(name)                                                    \
		StepTransition<styles::StyleProperty::name>(target, dt, anyActive);
		mc_animatableProperties
#undef mc_animatable

		Style = target;

		if (anyActive)
		{
			if (auto* ctx = getCtx(); ctx->_currentSurface != nullptr)
			{
				ctx->_currentSurface->MarkAnimating();
			}
		}
	}

	auto Node::FindTransition(styles::StyleProperty prop) -> ActiveTransition*
	{
		for (auto& transition : Transitions)
		{
			if (transition.Prop == prop)
			{
				return &transition;
			}
		}
		return nullptr;
	}

	void Node::RemoveTransition(styles::StyleProperty prop)
	{
		std::erase_if(
			Transitions,
			[prop](const ActiveTransition& t) -> auto
			{ return t.Prop == prop; }
		);
	}

	auto Node::BuildNodeTree(const Element& element) -> std::unique_ptr<Node>
	{
		auto node = std::make_unique<Node>();
		node->Id = nextNodeId++;
		node->Key = element.Key;
		node->Declared.Assign(element.Style);

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
			auto overflow = Style.Get<styles::StyleProperty::Overflow>();
			bool clips = overflow == OverflowType::Hidden ||
						 overflow == OverflowType::Scroll;
			bool scrolls = overflow == OverflowType::Scroll;

			if (clips)
			{
				canvas.PushClip(x, y, w, h);
			}

			auto resolveLength = [](const Length& length, float basis) -> float
			{
				return length.Unit == LengthUnit::Percent
						   ? length.Value * basis / 100.0F
						   : length.Value;
			};

			float radiusBasis = std::min(w, h);

			cmds::CornerRadii radius{
				.TopLeft = resolveLength(
					Style.Get<styles::StyleProperty::BorderRadiusTopLeft>(),
					radiusBasis
				),
				.TopRight = resolveLength(
					Style.Get<styles::StyleProperty::BorderRadiusTopRight>(),
					radiusBasis
				),
				.BottomRight = resolveLength(
					Style.Get<styles::StyleProperty::BorderRadiusBottomRight>(),
					radiusBasis
				),
				.BottomLeft = resolveLength(
					Style.Get<styles::StyleProperty::BorderRadiusBottomLeft>(),
					radiusBasis
				),
			};

			float radiusScale = 1.0F;
			if (radius.TopLeft + radius.TopRight > 0)
			{
				radiusScale = std::min(
					radiusScale,
					w / (radius.TopLeft + radius.TopRight)
				);
			}
			if (radius.BottomLeft + radius.BottomRight > 0)
			{
				radiusScale = std::min(
					radiusScale,
					w / (radius.BottomLeft + radius.BottomRight)
				);
			}
			if (radius.TopLeft + radius.BottomLeft > 0)
			{
				radiusScale = std::min(
					radiusScale,
					h / (radius.TopLeft + radius.BottomLeft)
				);
			}
			if (radius.TopRight + radius.BottomRight > 0)
			{
				radiusScale = std::min(
					radiusScale,
					h / (radius.TopRight + radius.BottomRight)
				);
			}
			if (radiusScale < 1.0F)
			{
				radius.TopLeft *= radiusScale;
				radius.TopRight *= radiusScale;
				radius.BottomRight *= radiusScale;
				radius.BottomLeft *= radiusScale;
			}

			cmds::EdgeSizes border{
				.Top =
					resolveLength(Style.Get<styles::StyleProperty::BorderTop>(), h),
				.Right = resolveLength(
					Style.Get<styles::StyleProperty::BorderRight>(),
					w
				),
				.Bottom = resolveLength(
					Style.Get<styles::StyleProperty::BorderBottom>(),
					h
				),
				.Left =
					resolveLength(Style.Get<styles::StyleProperty::BorderLeft>(), w),
			};

			canvas.DrawRect(
				x,
				y,
				w,
				h,
				Style.Get<styles::StyleProperty::BackgroundColor>(),
				RectStyle{
					.Radius = radius,
					.Border = border,
					.BorderColor =
						Style.Get<styles::StyleProperty::BorderColor>(),
				}
			);

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
				Style.Get<styles::StyleProperty::TextColor>()
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
		oldNode->Declared.Assign(newElement->Style);

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
		return n.IsBox() &&
			   n.Style.Get<styles::StyleProperty::Overflow>() !=
				   OverflowType::Visible;
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
		if (node->IsBox() &&
			node->Style.Get<styles::StyleProperty::Overflow>() ==
				OverflowType::Scroll)
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
		if (id == 0)
		{
			return nullptr;
		}
		
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