#include "core/Node.h"
#include "core/HookStore.h"
#include "core/NodeRef.h"
#include "gfx/Decoration.h"
#include "style/StyleProperty.h"
#include "text/Measure.h"
#include "text/TextGeometry.h"
#include "text/TextLayout.h"
#include "text/TextLayoutBuilder.h"
#include "style/StyleApply.h"
#include "utils/ScopedProfiler.h"
#include "yoga/YGNode.h"
#include "yoga/YGNodeLayout.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <optional>
#include <ranges>
#include <vector>
#include "core/Context.h"
#include "core/Surface.h"

namespace mocca::detail
{
	Node::~Node()
	{
		getCtx()->_nodes.erase(Id);

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
		mc_profileFunction();
		if (YogaNode != nullptr)
		{
			if (YogaDirty)
			{
				ApplyLayoutStyles();
				YogaDirty = false;
			}

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

	static auto AccumulateSubtreeBounds(Node& n, float parentX, float parentY)
		-> Rectangle
	{
		float x = parentX;
		float y = parentY;
		if (n.YogaNode != nullptr)
		{
			x += YGNodeLayoutGetLeft(n.YogaNode);
			y += YGNodeLayoutGetTop(n.YogaNode);
		}

		n.LayoutPos = {.X = x, .Y = y};

		Rectangle bounds{};
		if (n.YogaNode != nullptr)
		{
			bounds = {
				.X = x,
				.Y = y,
				.Width = YGNodeLayoutGetWidth(n.YogaNode),
				.Height = YGNodeLayoutGetHeight(n.YogaNode),
			};
		}

		for (auto& child : n.Children)
		{
			if (child)
			{
				bounds = bounds.Union(AccumulateSubtreeBounds(*child, x, y));
			}
		}

		n.SubtreeBounds = bounds;
		return bounds;
	}

	void Node::UpdateSubtreeBounds()
	{
		mc_profileFunction();
		AccumulateSubtreeBounds(*this, 0.0F, 0.0F);
	}

	void Node::ApplyLayoutStyles() const
	{
		mc_profileFunction();
#define mc_styleGroup LayoutSizing

#define mc_styleProperty(name, ...)                                            \
	styles::detail::applying::layout::apply##name(                             \
		YogaNode,                                                              \
		Style.mc_styleGroup.name                                               \
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

	void Node::ComputeStyle(
		styles::ComputedStyle& parentComputed,
		double dt,
		uint8_t parentChangedGroups
	)
	{
		mc_profileFunction();

		bool needGoal = StyleDirty || !Styled ||
						(DepGroups & parentChangedGroups) != 0;
		bool animating = !Transitions.empty();

		uint8_t changed = 0;

		if (needGoal || animating)
		{
			styles::ComputedStyle target =
				styles::ComputedStyle::Cascade(Declared, parentComputed);

			if (!Styled)
			{
				Style = target;
				Styled = true;
				changed = styles::AllGroups;
			}
			else
			{
				changed = styles::ComputedStyle::ChangedGroups(Style, target);
				AdvanceTransitions(target, dt);
			}
		}

		ChangedGroups = changed;
		StyleDirty = false;

		if ((changed & styles::DecorationGroups) != 0)
		{
			DecoDirty = true;
		}

		if ((changed & styles::LayoutGroups) != 0)
		{
			YogaDirty = true;
		}

		if (IsText() && YogaNode != nullptr &&
			(changed & styles::InheritedGroupBit) != 0)
		{
			YGNodeMarkDirty(YogaNode);
		}

		for (auto& child : Children)
		{
			if (child)
			{
				child->ComputeStyle(Style, dt, changed);
			}
		}
	}

	void Node::AdvanceTransitions(styles::ComputedStyle& target, double dt)
	{
		if (Transitions.empty() &&
			target.Get<styles::StyleProperty::Transition>() == 0)
		{
			Style = target;
			return;
		}

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
			[prop](const ActiveTransition& t) -> auto { return t.Prop == prop; }
		);
	}

	auto Node::BuildNodeTree(const Element& element) -> std::unique_ptr<Node>
	{
		mc_profileFunction();

		auto node = std::make_unique<Node>();
		node->Id = nextNodeId++;
		node->Key = element.Key;
		node->Declared.Assign(element.Style);
		node->DepGroups = node->Declared.DependentGroups();
		node->Surface = getCtx()->_currentSurface;

		getCtx()->_nodes[node->Id] = node.get();

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
					node->Kind = arm;
				}
				else if constexpr (std::is_same_v<T, ComponentElement>)
				{
					node->Kind = ComponentElement{
						.Fn = arm.Fn,
						.Props = arm.Props,
					};
				}
			},
			element.Node
		);

		node->Events = element.Events;
		node->Focus = element.Focus;

		if (!node->IsComponent())
		{
			node->YogaNode = YGNodeNew();
			YGNodeSetContext(node->YogaNode, node.get());
			if (node->IsText())
			{
				YGNodeSetMeasureFunc(
					node->YogaNode,
					&mocca::mtext::detail::measureFunc
				);
			}
		}

		return node;
	}

	auto Node::GetX() const -> float
	{
		return LayoutPos.X;
	}

	auto Node::GetY() const -> float
	{
		return LayoutPos.Y;
	}

	struct NodeSpace
	{
		bool Clips = false;
		bool Scrolls = false;
		bool HasTransform = false;
		bool Invertible = true;
		gfx::Matrix NodeTransform = gfx::Matrix::Identity();
		gfx::Matrix ParentToLocal = gfx::Matrix::Identity();
	};

	auto computeNodeSpace(const Node& node) -> NodeSpace
	{
		NodeSpace space{};

		if (node.IsBox())
		{
			auto overflow = node.Style.Get<styles::StyleProperty::Overflow>();
			space.Clips = overflow == styles::OverflowType::Hidden ||
						  overflow == styles::OverflowType::Scroll;
			space.Scrolls = overflow == styles::OverflowType::Scroll;
		}

		if (auto inverse = space.NodeTransform.Invert(); inverse)
		{
			space.ParentToLocal = *inverse;
			space.HasTransform = !space.NodeTransform.IsIdentity();
		}
		else
		{
			space.Invertible = false;
		}

		return space;
	}

	auto narrowCull(
		const Rectangle& parentCull,
		const NodeSpace& space,
		const Rectangle& box,
		const Vector2& scroll
	) -> Rectangle
	{
		if (!space.Invertible)
		{
			return {};
		}

		Rectangle cull = space.ParentToLocal.MapRect(parentCull);
		if (space.Clips)
		{
			cull = cull.Intersect(box);
		}
		if (space.Scrolls)
		{
			cull.X += scroll.X;
			cull.Y += scroll.Y;
		}
		return cull;
	}

	auto mapPointToChild(
		const NodeSpace& space,
		const Rectangle& box,
		const Vector2& scroll,
		Vector2 parentPoint
	) -> std::optional<Vector2>
	{
		if (!space.Invertible)
		{
			return std::nullopt;
		}

		Vector2 local = space.ParentToLocal.MapPoint(parentPoint);
		if (space.Clips && !box.Contains(local))
		{
			return std::nullopt;
		}
		if (space.Scrolls)
		{
			local.X += scroll.X;
			local.Y += scroll.Y;
		}
		return local;
	}

	void paintBorder(
		Canvas& canvas,
		const gfx::BoxDecoration& deco,
		const RRect& outer
	)
	{
		auto paint = gfx::Paint::Fill(deco.BorderColor);
		RRect inner = outer.Inset(deco.Border);

		if (inner.IsEmpty())
		{
			canvas.DrawRRect(outer, paint);
		}
		else
		{
			canvas.DrawDRRect(outer, inner, paint);
		}
	}

	void paintDecoration(
		Canvas& canvas,
		const gfx::BoxDecoration& deco,
		const Rectangle& rect
	)
	{
		RRect outer{
			.Rect = rect,
			.Radii = deco.Radius,
		};

		if (deco.HasFill)
		{
			canvas.DrawRRect(outer, deco.Fill);
		}
		if (deco.HasImage)
		{
			for (const auto& image : deco.Images)
			{
				canvas.DrawImage(image, outer);
			}
		}
		if (deco.HasBorder)
		{
			paintBorder(canvas, deco, outer);
		}
	}

	auto Node::ResolveDecoration(float w, float h) -> const gfx::BoxDecoration*
	{
		bool sizeChanged = w != DecoWidth || h != DecoHeight;
		if (DecoDirty ||
			(Deco != nullptr && Deco->SizeDependent && sizeChanged))
		{
			_resolveDecoration(w, h);
		}

		return Deco.get();
	}

	void Node::_resolveDecoration(float w, float h)
	{
		gfx::BoxDecoration deco{};
		const styles::detail::applying::paint::PaintContext ctx{
			.Width = w,
			.Height = h,
		};

#define mc_styleProperty(name, ...)                                            \
	styles::detail::applying::paint::apply##name(deco, Style.Render.name, ctx);
		mc_renderPropertiesReset
#undef mc_styleProperty

			namespace paint = styles::detail::applying::paint;
		paint::applyBorderTop(deco, Style.LayoutSpacing.BorderTop, ctx);
		paint::applyBorderRight(deco, Style.LayoutSpacing.BorderRight, ctx);
		paint::applyBorderBottom(deco, Style.LayoutSpacing.BorderBottom, ctx);
		paint::applyBorderLeft(deco, Style.LayoutSpacing.BorderLeft, ctx);
		paint::normalizeRadius(deco, w, h);

		Deco = deco.IsEmpty()
				   ? nullptr
				   : std::make_unique<gfx::BoxDecoration>(std::move(deco));
		DecoWidth = w;
		DecoHeight = h;
		DecoDirty = false;
	}

	auto lineDecorationY(
		gfx::TextDecorationKind kind,
		const mtext::TextLayout& layout,
		const mtext::TextSpanBox& span
	) -> float
	{
		switch (kind)
		{
		case gfx::TextDecorationKind::Overline:
			return span.Baseline - layout.Ascent;
		case gfx::TextDecorationKind::Strikethrough:
			return span.Baseline - (layout.Ascent * 0.4F);
		case gfx::TextDecorationKind::Selection:
		case gfx::TextDecorationKind::Underline:
		default:
			return span.Baseline + (layout.Descent * 0.5F);
		}
	}

	void paintTextContent(Node& node, Canvas& canvas, const Rectangle& box)
	{
		mc_profileFunction();

		const auto& textElement = std::get<TextElement>(node.Kind);
		const std::string& content = textElement.Content;

		float x = box.X;
		float y = box.Y;
		float w = box.Width;

		float caretWidth = textElement.Caret
							   ? std::max(0.0F, textElement.Caret->Width)
							   : 0.0F;
		float textWidth = std::max(0.0F, w - caretWidth);

		auto layout = mtext::detail::TextLayoutBuilder::ResolveTextLayout(
			node.Style,
			content,
			textWidth,
			true
		);

		auto align = node.Style.Get<styles::StyleProperty::TextAlign>();

		auto emit =
			[&](float rx, float ry, float rw, float rh, Color color) -> void
		{
			canvas.DrawRect(
				{.X = x + rx, .Y = y + ry, .Width = rw, .Height = rh},
				gfx::Paint::Fill(color)
			);
		};

		if (textElement.Decorations)
		{
			for (const auto& decoration : *textElement.Decorations)
			{
				if (decoration.Kind != gfx::TextDecorationKind::Selection)
				{
					continue;
				}

				for (const auto& rect : mtext::TextGeometry::SelectionRects(
						 *layout,
						 decoration.Start,
						 decoration.End,
						 textWidth,
						 align
					 ))
				{
					emit(
						rect.X,
						rect.Y,
						rect.Width,
						rect.Height,
						decoration.Color
					);
				}
			}
		}

		canvas.DrawText(
			cmds::DrawTextCmd{
				.Position = {.X = x, .Y = y},
				.Content = content,
				.Paint = gfx::Paint::Fill(
					node.Style.Get<styles::StyleProperty::TextColor>()
				),
				.Size = node.Style.Get<styles::StyleProperty::FontSize>().Value,
				.Face = layout->Face,
				.Layout = layout,
				.BoxWidth = textWidth,
				.Align = align,
			}
		);

		if (textElement.Decorations)
		{
			for (const auto& decoration : *textElement.Decorations)
			{
				if (decoration.Kind == gfx::TextDecorationKind::Selection)
				{
					continue;
				}

				for (const auto& span : mtext::TextGeometry::SpanBoxes(
						 *layout,
						 decoration.Start,
						 decoration.End,
						 textWidth,
						 align
					 ))
				{
					emit(
						span.X,
						lineDecorationY(decoration.Kind, *layout, span),
						span.Width,
						decoration.Thickness,
						decoration.Color
					);
				}
			}
		}

		if (textElement.Caret)
		{
			const auto& caret = *textElement.Caret;
			double now = getCtx()->TimeMs();

			if (caret.BlinkResetToken != node.BlinkToken)
			{
				node.BlinkToken = caret.BlinkResetToken;
				node.BlinkEpochMs = now;
			}

			bool visible = caret.BlinkPeriodMs == 0;
			if (!visible)
			{
				auto period = static_cast<double>(caret.BlinkPeriodMs);
				visible = std::fmod(now - node.BlinkEpochMs, period * 2.0) <
						  period;
				getCtx()->RequestRepaint();
			}

			if (visible)
			{
				auto position = mtext::TextPosition{
					.Offset = caret.Offset,
					.Affinity = caret.Affinity,
				};
				auto box = mtext::TextGeometry::CaretAt(
					*layout,
					position,
					textWidth,
					align
				);
				if (box.Height > 0.0F)
				{
					float drawX = std::round(x + box.X);
					float drawY = std::round(y + box.Y);
					float drawW = std::max(1.0F, std::round(caret.Width));
					float drawH = std::round(box.Height);
					canvas.DrawRect(
						{
							.X = drawX,
							.Y = drawY,
							.Width = drawW,
							.Height = drawH,
						},
						gfx::Paint::Fill(caret.Color)
					);
				}
			}
		}
	}

	void paintNode(
		Node& node,
		Canvas& canvas,
		const Rectangle& cull,
		float x,
		float y,
		float w,
		float h
	)
	{
		mc_profileFunction();

		Rectangle box{.X = x, .Y = y, .Width = w, .Height = h};
		NodeSpace space = computeNodeSpace(node);

		if (!space.Invertible)
		{
			return;
		}

		if (space.HasTransform)
		{
			canvas.Save();
			canvas.Concat(space.NodeTransform);
		}

		if (const auto* deco = node.ResolveDecoration(w, h))
		{
			paintDecoration(canvas, *deco, box);
		}

		if (node.IsText())
		{
			paintTextContent(node, canvas, box);
		}

		bool scoped = space.Clips || space.Scrolls;
		if (scoped)
		{
			canvas.Save();
			if (space.Clips)
			{
				canvas.ClipRect(box);
			}
			if (space.Scrolls)
			{
				canvas.Translate(-node.ScrollOffset.X, -node.ScrollOffset.Y);
			}
		}

		Rectangle childCull =
			narrowCull(cull, space, box, node.ScrollOffset);
		for (auto& child : node.Children)
		{
			child->Paint(canvas, childCull);
		}

		if (scoped)
		{
			canvas.Restore();
		}
		if (space.HasTransform)
		{
			canvas.Restore();
		}
	}

	void Node::Paint(Canvas& canvas, const Rectangle& cull)
	{
		mc_profileFunction();

		if (!SubtreeBounds.IsEmpty() && !SubtreeBounds.Intersects(cull))
		{
			return;
		}

		if (IsComponent())
		{
			for (auto& child : Children)
			{
				child->Paint(canvas, cull);
			}
			return;
		}

		paintNode(
			*this,
			canvas,
			cull,
			GetX(),
			GetY(),
			YGNodeLayoutGetWidth(YogaNode),
			YGNodeLayoutGetHeight(YogaNode)
		);
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

	auto Node::ReconcileChildrenSlow(
		std::vector<std::unique_ptr<Node>> oldChildren,
		std::span<const Element> childElements,
		Node* parent
	) -> std::vector<std::unique_ptr<Node>>
	{
		mc_profileFunction();

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
		newChildren.reserve(childElements.size());

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
				getCtx()->_store->RemoveComponent(n.get());
			}
		}
		for (size_t i = unkeyedCursor; i < oldUnkeyed.size(); i++)
		{
			getCtx()->_store->RemoveComponent(oldUnkeyed[i].get());
		}

		return newChildren;
	}

	auto Node::ReconcileChildren(
		std::vector<std::unique_ptr<Node>> old,
		std::span<const Element> kids,
		Node* parent
	) -> std::vector<std::unique_ptr<Node>>
	{
		mc_profileFunction();

		size_t common = std::min(old.size(), kids.size());
		bool aligned = true;
		for (size_t i = 0; i < common; ++i)
		{
			if (!old[i] || old[i]->Key != kids[i].Key)
			{
				aligned = false;
				break;
			}
		}

		if (aligned)
		{
			std::vector<std::unique_ptr<Node>> out;
			out.reserve(kids.size());
			for (size_t i = 0; i < kids.size(); ++i)
			{
				std::unique_ptr<Node> prev;
				if (i < old.size())
				{
					prev = std::move(old[i]);
				}
				if (auto n = Reconcile(std::move(prev), &kids[i]))
				{
					n->Parent = parent;
					out.push_back(std::move(n));
				}
			}
			for (size_t i = kids.size(); i < old.size(); ++i)
			{
				if (old[i])
				{
					getCtx()->_store->RemoveComponent(old[i].get());
				}
			}
			return out;
		}

		return ReconcileChildrenSlow(std::move(old), kids, parent);
	}

	auto Node::Reconcile(
		std::unique_ptr<Node> oldNode,
		const Element* newElement
	) -> std::unique_ptr<Node>
	{
		mc_profileFunction();
		if (newElement == nullptr && oldNode == nullptr)
		{
			return nullptr;
		}
		if (newElement == nullptr) // oldNode != nullptr implied
		{
			getCtx()->_store->RemoveComponent(oldNode.get());
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
			getCtx()->_store->RemoveComponent(oldNode.get());
			oldNode = BuildNodeTree(*newElement);
		}

		if (oldNode->IsText())
		{
			const auto& incoming = std::get<TextElement>(newElement->Node);
			const auto& current = std::get<TextElement>(oldNode->Kind);

			bool contentChanged = current.Content != incoming.Content;
			bool caretWidthChanged = false;
			if (current.Caret.has_value() != incoming.Caret.has_value())
			{
				caretWidthChanged = true;
			}
			else if (current.Caret.has_value() && incoming.Caret.has_value())
			{
				caretWidthChanged = current.Caret->Width !=
									incoming.Caret->Width;
			}

			bool overlayChanged = current.Caret != incoming.Caret ||
								  current.Decorations != incoming.Decorations;

			if (contentChanged || overlayChanged)
			{
				oldNode->Kind = incoming;

				if (contentChanged || caretWidthChanged)
				{
					YGNodeMarkDirty(oldNode->YogaNode);
				}
			}

			YGNodeSetMeasureFunc(
				oldNode->YogaNode,
				&mocca::mtext::detail::measureFunc
			);
		}

		oldNode->Events = newElement->Events;
		oldNode->Focus = newElement->Focus;
		if (oldNode->Declared.Assign(newElement->Style))
		{
			oldNode->StyleDirty = true;
			oldNode->DepGroups = oldNode->Declared.DependentGroups();
		}

		if (oldNode->IsComponent())
		{
			if (const auto* comp =
					std::get_if<ComponentElement>(&newElement->Node))
			{
				oldNode->Kind =
					ComponentElement{.Fn = comp->Fn, .Props = comp->Props};
			}
		}

		if (newElement->Ref != nullptr)
		{
			newElement->Ref->_id = oldNode->Id;
			newElement->Ref->_surface = getCtx()->_currentSurface;
			newElement->Ref->_node = oldNode.get();
		}

		std::optional<Element> produced;
		std::span<const Element> kids;

		if (const auto* box = std::get_if<BoxElement>(&newElement->Node))
		{
			kids = box->Children;
		}
		else if (
			const auto* comp = std::get_if<ComponentElement>(&newElement->Node)
		)
		{
			produced.emplace(
				Element::Render(comp->Fn, comp->Props, oldNode->Id)
			);
			kids = std::span<const Element>(&*produced, 1);
		}

		oldNode->Children = ReconcileChildren(
			std::move(oldNode->Children),
			kids,
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

	auto hitTestImpl(Node* node, float x, float y) -> Node*
	{
		if (node == nullptr)
		{
			return nullptr;
		}

		Vector2 parentPoint = {.X = x, .Y = y};

		if (!node->SubtreeBounds.Contains(parentPoint))
		{
			return nullptr;
		}

		Rectangle box{};
		if (!node->IsComponent())
		{
			box = layoutRect(*node);
		}

		NodeSpace space = computeNodeSpace(*node);
		if (!space.Invertible)
		{
			return nullptr;
		}

		bool inside = !node->IsComponent() && box.Contains(parentPoint);

		auto childPoint =
			mapPointToChild(space, box, node->ScrollOffset, parentPoint);
		if (!childPoint)
		{
			return nullptr;
		}

		for (auto& child : std::views::reverse(node->Children))
		{
			if (Node* hit =
					hitTestImpl(child.get(), childPoint->X, childPoint->Y))
			{
				return hit;
			}
		}

		return inside ? node : nullptr;
	}

	auto Node::HitTest(Node* root, float x, float y) -> Node*
	{
		mc_profileFunction();
		return hitTestImpl(root, x, y);
	}

	// we should also add an id -> node* mapping in the surface later
	auto Node::FindNodeById(Node* root, NodeId id) -> Node*
	{
		if (getCtx() == nullptr)
		{
			return nullptr;
		}

		return getCtx()->FindNodeById(id);
	}
}