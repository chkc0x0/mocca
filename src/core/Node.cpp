#include "core/Node.h"
#include "core/HookStore.h"
#include "core/NodeRef.h"
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

	auto resolveLength(const Length& length, float basis) -> float
	{
		return length.Unit == LengthUnit::Percent
				   ? length.Value * basis / 100.0F
				   : length.Value;
	}

	auto
	resolveCornerRadii(const styles::ComputedStyle& style, float w, float h)
		-> CornerRadii
	{
		float radiusBasis = std::min(w, h);

		CornerRadii radius{
			.TopLeft = resolveLength(
				style.Get<styles::StyleProperty::BorderRadiusTopLeft>(),
				radiusBasis
			),
			.TopRight = resolveLength(
				style.Get<styles::StyleProperty::BorderRadiusTopRight>(),
				radiusBasis
			),
			.BottomRight = resolveLength(
				style.Get<styles::StyleProperty::BorderRadiusBottomRight>(),
				radiusBasis
			),
			.BottomLeft = resolveLength(
				style.Get<styles::StyleProperty::BorderRadiusBottomLeft>(),
				radiusBasis
			),
		};

		float radiusScale = 1.0F;
		if (radius.TopLeft + radius.TopRight > 0)
		{
			radiusScale =
				std::min(radiusScale, w / (radius.TopLeft + radius.TopRight));
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
			radiusScale =
				std::min(radiusScale, h / (radius.TopLeft + radius.BottomLeft));
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

		return radius;
	}

	auto resolveBorder(const styles::ComputedStyle& style, float w, float h)
		-> EdgeSizes
	{
		return {
			.Top =
				resolveLength(style.Get<styles::StyleProperty::BorderTop>(), h),
			.Right = resolveLength(
				style.Get<styles::StyleProperty::BorderRight>(),
				w
			),
			.Bottom = resolveLength(
				style.Get<styles::StyleProperty::BorderBottom>(),
				h
			),
			.Left = resolveLength(
				style.Get<styles::StyleProperty::BorderLeft>(),
				w
			),
		};
	}

	void paintDecorations(
		const Node& node,
		Canvas& canvas,
		float x,
		float y,
		float w,
		float h
	)
	{
		const auto& style = node.Style;
		auto radius = resolveCornerRadii(style, w, h);
		auto border = resolveBorder(style, w, h);
		auto background = style.Get<styles::StyleProperty::BackgroundColor>();
		auto borderColor = style.Get<styles::StyleProperty::BorderColor>();
		uint32_t imageRef = style.Get<styles::StyleProperty::BackgroundImage>();

		RRect rrect{
			.Rect = {.X = x, .Y = y, .Width = w, .Height = h},
			.Radii = radius,
		};
		float borderWidth = std::max({
			border.Top,
			border.Right,
			border.Bottom,
			border.Left,
		});
		bool hasBorder = borderWidth > 0 && borderColor.A > 0;
		gfx::Paint borderPaint = gfx::Paint::Stroke(borderColor, borderWidth);

		canvas.DrawRRect(rrect, gfx::Paint::Fill(background));

		if (imageRef != 0)
		{
			const auto& image = styles::detail::resolveBackgroundImage(
				imageRef
			);
			for (const auto& layer : image.Layers)
			{
				canvas.DrawImage(layer.Texture, rrect);
			}
		}

		if (hasBorder)
		{
			canvas.DrawRRect(rrect, borderPaint);
		}
	}

	auto lineDecorationY(
		mtext::TextDecorationKind kind,
		const mtext::TextLayout& layout,
		const mtext::TextSpanBox& span
	) -> float
	{
		switch (kind)
		{
		case mtext::TextDecorationKind::Overline:
			return span.Baseline - layout.Ascent;
		case mtext::TextDecorationKind::Strikethrough:
			return span.Baseline - (layout.Ascent * 0.4F);
		case mtext::TextDecorationKind::Selection:
		case mtext::TextDecorationKind::Underline:
		default:
			return span.Baseline + (layout.Descent * 0.5F);
		}
	}

	void
	paintText(Node& node, Canvas& canvas, float x, float y, float w, float h)
	{
		mc_profileFunction();

		const auto& textElement = std::get<TextElement>(node.Kind);
		const std::string& content = textElement.Content;

		canvas.DrawRect(
			{.X = x, .Y = y, .Width = w, .Height = h},
			gfx::Paint::Fill(
				node.Style.Get<styles::StyleProperty::BackgroundColor>()
			)
		);

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
				if (decoration.Kind != mtext::TextDecorationKind::Selection)
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
				if (decoration.Kind == mtext::TextDecorationKind::Selection)
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

	void paintBox(
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

		auto overflow = node.Style.Get<styles::StyleProperty::Overflow>();
		bool clips = overflow == styles::OverflowType::Hidden ||
					 overflow == styles::OverflowType::Scroll;
		bool scrolls = overflow == styles::OverflowType::Scroll;

		if (clips)
		{
			canvas.Save();
			canvas.ClipRect({.X = x, .Y = y, .Width = w, .Height = h});
		}

		paintDecorations(node, canvas, x, y, w, h);

		if (scrolls)
		{
			canvas.Save();
			canvas.Translate(-node.ScrollOffset.X, -node.ScrollOffset.Y);
		}

		Rectangle childCull = cull;
		if (clips)
		{
			childCull = childCull.Intersect(
				{.X = x, .Y = y, .Width = w, .Height = h}
			);
		}
		if (scrolls)
		{
			childCull.X += node.ScrollOffset.X;
			childCull.Y += node.ScrollOffset.Y;
		}

		for (auto& child : node.Children)
		{
			child->Paint(canvas, childCull);
		}

		if (scrolls)
		{
			canvas.Restore();
		}

		if (clips)
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

		float x = GetX();
		float y = GetY();
		float w = YGNodeLayoutGetWidth(YogaNode);
		float h = YGNodeLayoutGetHeight(YogaNode);

		if (IsBox())
		{
			paintBox(*this, canvas, cull, x, y, w, h);
		}
		else if (IsText())
		{
			paintText(*this, canvas, x, y, w, h);
		}
		else
		{
			for (auto& child : Children)
			{
				child->Paint(canvas, cull);
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

	auto clipsChildren(const Node& n) -> bool
	{
		return n.IsBox() && n.Style.Get<styles::StyleProperty::Overflow>() !=
								styles::OverflowType::Visible;
	}

	auto hitTestImpl(Node* node, float x, float y) -> Node*
	{
		if (node == nullptr)
		{
			return nullptr;
		}

		Vector2 childP = {.X = x, .Y = y};

		if (!node->SubtreeBounds.Contains(childP))
		{
			return nullptr;
		}

		bool inside = !node->IsComponent() &&
					  layoutRect(*node).Contains(childP);

		if (!inside && clipsChildren(*node))
		{
			return nullptr;
		}

		if (node->IsBox() &&
			node->Style.Get<styles::StyleProperty::Overflow>() ==
				styles::OverflowType::Scroll)
		{
			childP.X += node->ScrollOffset.X;
			childP.Y += node->ScrollOffset.Y;
		}

		for (auto& child : std::views::reverse(node->Children))
		{
			if (Node* hit = hitTestImpl(child.get(), childP.X, childP.Y))
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