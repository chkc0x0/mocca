#include "core/Node.h"
#include "core/HookStore.h"
#include "core/NodeRef.h"
#include "core/Part.h"
#include "input/Scroll.h"
#include "style/StyleBuilder.h"
#include "style/StyleProperty.h"
#include "text/Measure.h"
#include "text/TextGeometry.h"
#include "text/TextLayout.h"
#include "text/TextLayoutBuilder.h"
#include "style/StyleApply.h"
#include "style/PaintApply.h"
#include "style/StyleHelpers.h"
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
	Node::Node() = default;

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

	static auto ResolveTransformLength(
		const styles::Length& value,
		float basis,
		float emBasis
	) -> float
	{
		switch (value.Unit)
		{
		case styles::LengthUnit::Percent:
			return value.Value / 100.0F * basis;
		case styles::LengthUnit::Em:
			return value.Value * emBasis;
		case styles::LengthUnit::Rem:
			return value.Value * styles::RootFontSize;
		case styles::LengthUnit::Pixels:
			break;
		}

		return value.Value;
	}

	static auto ComputeNodeTransform(const Node& node, const Rectangle& box)
		-> gfx::Matrix
	{
		if (node.IsComponent())
		{
			return gfx::Matrix::Identity();
		}

		const auto& transform = node.Style.Transform;
		if (transform.Translate == styles::Vector2Length{} &&
			transform.Scale == Vector2{1.0F, 1.0F} && transform.Rotate == 0.0F)
		{
			return gfx::Matrix::Identity();
		}

		float emBasis = node.Style.Inherited.FontSize.Value;
		auto resolveX = [&](const styles::Length& value) -> float
		{ return ResolveTransformLength(value, box.Width, emBasis); };
		auto resolveY = [&](const styles::Length& value) -> float
		{ return ResolveTransformLength(value, box.Height, emBasis); };

		float originX = resolveX(transform.TransformOrigin.X);
		float originY = resolveY(transform.TransformOrigin.Y);

		auto translate = gfx::Matrix::Translate(
			resolveX(transform.Translate.X),
			resolveY(transform.Translate.Y)
		);
		auto rotate = gfx::Matrix::Rotate(transform.Rotate);
		auto scale = gfx::Matrix::Scale(transform.Scale.X, transform.Scale.Y);

		gfx::Matrix core = gfx::Matrix::Identity();
		switch (transform.TransformOrder)
		{
		case styles::TransformOrderType::TRS:
			core = gfx::Matrix::Concat(
				translate,
				gfx::Matrix::Concat(rotate, scale)
			);
			break;
		case styles::TransformOrderType::TSR:
			core = gfx::Matrix::Concat(
				translate,
				gfx::Matrix::Concat(scale, rotate)
			);
			break;
		case styles::TransformOrderType::RTS:
			core = gfx::Matrix::Concat(
				rotate,
				gfx::Matrix::Concat(translate, scale)
			);
			break;
		case styles::TransformOrderType::RST:
			core = gfx::Matrix::Concat(
				rotate,
				gfx::Matrix::Concat(scale, translate)
			);
			break;
		case styles::TransformOrderType::STR:
			core = gfx::Matrix::Concat(
				scale,
				gfx::Matrix::Concat(translate, rotate)
			);
			break;
		case styles::TransformOrderType::SRT:
			core = gfx::Matrix::Concat(
				scale,
				gfx::Matrix::Concat(rotate, translate)
			);
			break;
		}

		auto localTransform = gfx::Matrix::Concat(
			gfx::Matrix::Translate(originX, originY),
			gfx::Matrix::Concat(
				core,
				gfx::Matrix::Translate(-originX, -originY)
			)
		);

		return gfx::Matrix::Concat(
			gfx::Matrix::Translate(box.X, box.Y),
			gfx::Matrix::Concat(
				localTransform,
				gfx::Matrix::Translate(-box.X, -box.Y)
			)
		);
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

		Rectangle rect{};
		if (n.YogaNode != nullptr)
		{
			rect = {
				.X = x,
				.Y = y,
				.Width = YGNodeLayoutGetWidth(n.YogaNode),
				.Height = YGNodeLayoutGetHeight(n.YogaNode),
			};
		}

		gfx::Matrix transform = ComputeNodeTransform(n, rect);
		Rectangle bounds = transform.MapRect(rect);

		for (auto& child : n.Children)
		{
			if (child)
			{
				bounds = bounds.Union(transform.MapRect(
					AccumulateSubtreeBounds(*child, x, y)
				));
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

		if (Parts != nullptr || !PartTransitions.empty())
		{
			bool anyPartActive = false;
			AdvancePartTransitions(dt, anyPartActive);

			if (anyPartActive)
			{
				if (auto* ctx = getCtx(); ctx->_currentSurface != nullptr)
				{
					ctx->_currentSurface->MarkAnimating();
				}
			}
		}

		for (auto& child : Children)
		{
			if (child)
			{
				child->ComputeStyle(Style, dt, changed);
			}
		}
	}

	auto findActiveTransition(
		std::vector<ActiveTransition>& transitions,
		styles::StyleProperty prop
	) -> ActiveTransition*
	{
		for (auto& transition : transitions)
		{
			if (transition.Prop == prop)
			{
				return &transition;
			}
		}
		return nullptr;
	}

	void removeActiveTransition(
		std::vector<ActiveTransition>& transitions,
		styles::StyleProperty prop
	)
	{
		std::erase_if(
			transitions,
			[prop](const ActiveTransition& t) -> auto { return t.Prop == prop; }
		);
	}

	template <styles::StyleProperty P>
	void StepOne(
		styles::ComputedStyle& current,
		std::vector<ActiveTransition>& transitions,
		styles::ComputedStyle& target,
		double dt,
		bool& anyActive
	)
	{
		uint32_t transitionRef =
			target.Get<styles::StyleProperty::Transition>();
		if (transitionRef == 0)
		{
			removeActiveTransition(transitions, P);
			return;
		}

		using Value = styles::PropTraits<P>::Value;

		const styles::TransitionList& transitionList =
			styles::detail::resolveTransition(transitionRef);

		const styles::TransitionType* spec =
			transitionList.Find(styles::PropertySet::Set(P));

		if (spec == nullptr)
		{
			removeActiveTransition(transitions, P);
			return;
		}

		Value goal = target.Get<P>();
		ActiveTransition* tr = findActiveTransition(transitions, P);

		if (tr == nullptr)
		{
			Value present = current.Get<P>();
			if (present == goal)
			{
				return;
			}

			ActiveTransition fresh;
			fresh.Prop = P;
			fresh.From = styles::detail::encodeStyleEntry(P, present);
			fresh.Goal = styles::detail::encodeStyleEntry(P, goal);
			fresh.Duration = spec->Duration;
			fresh.Curve = spec->Curve;
			transitions.push_back(fresh);
			tr = &transitions.back();
		}
		else if (styles::getStyleEntryValue<Value>(tr->Goal) != goal)
		{
			tr->From = styles::detail::encodeStyleEntry(P, current.Get<P>());
			tr->Goal = styles::detail::encodeStyleEntry(P, goal);
			tr->Elapsed = 0.0F;
			tr->Progress = 0.0F;
			tr->Spring = {};
			tr->Duration = spec->Duration;
			tr->Curve = spec->Curve;
		}

		float t = 1.0F;
		bool settled = false;
		if (tr->Curve.Kind == anim::CurveKind::Spring)
		{
			tr->Progress = anim::springStep(
				tr->Progress,
				1.0F,
				tr->Spring,
				tr->Curve,
				static_cast<float>(dt)
			);
			t = tr->Progress;
			settled = anim::springSettled(tr->Progress, 1.0F, tr->Spring);
		}
		else
		{
			tr->Elapsed += static_cast<float>(dt);
			float p = tr->Duration > 0.0F
						  ? std::min(tr->Elapsed / tr->Duration, 1.0F)
						  : 1.0F;
			t = anim::sample(tr->Curve, p);
			settled = tr->Elapsed >= tr->Duration;
		}

		target.Blend(P, tr->From, tr->Goal, t);

		if (settled)
		{
			target.Set<P>(goal);
			removeActiveTransition(transitions, P);
		}
		else
		{
			anyActive = true;
		}
	}

	void advanceAll(
		styles::ComputedStyle& current,
		std::vector<ActiveTransition>& transitions,
		styles::ComputedStyle& target,
		double dt,
		bool& anyActive
	)
	{
#define mc_animatable(name)                                                    \
	StepOne<styles::StyleProperty::name>(                                      \
		current,                                                               \
		transitions,                                                           \
		target,                                                                \
		dt,                                                                    \
		anyActive                                                              \
	);
		mc_animatableProperties
#undef mc_animatable
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

		advanceAll(Style, Transitions, target, dt, anyActive);

		Style = target;

		if (anyActive)
		{
			if (auto* ctx = getCtx(); ctx->_currentSurface != nullptr)
			{
				ctx->_currentSurface->MarkAnimating();
			}
		}
	}

	void Node::AdvancePartTransitions(double dt, bool& anyActive)
	{
		mc_profileFunction();

		if (Parts == nullptr && PartTransitions.empty())
		{
			return;
		}

		std::vector<std::uint64_t> visited;
		visited.reserve(PartTransitions.size() + 8);

		auto advanceParts = [&](auto&& self,
									  std::span<const Part> parts,
									  const styles::ComputedStyle& parentStyle,
									  std::uint64_t parentKey) -> void
		{
			for (std::size_t i = 0; i < parts.size(); ++i)
			{
				const Part& part = parts[i];
				std::uint64_t key = partKey(parentKey, part, i);
				visited.push_back(key);

				styles::ComputedStyle target = styles::ComputedStyle::Cascade(
					part.Style.Entries(),
					parentStyle
				);

				PartTransitionState& state = PartTransitions[key];

				if (!state.Initialized)
				{
					state.Current = target;
					state.Transitions.clear();
					state.Initialized = true;
				}
				else if (
					state.Transitions.empty() &&
					target.Get<styles::StyleProperty::Transition>() == 0
				)
				{
					state.Current = target;
				}
				else
				{
					advanceAll(
						state.Current,
						state.Transitions,
						target,
						dt,
						anyActive
					);
					state.Current = target;
				}

				self(self, part.Parts, target, key);
			}
		};

		if (Parts != nullptr)
		{
			advanceParts(
				advanceParts,
				*Parts,
				Style,
				static_cast<std::uint64_t>(Id)
			);
		}

		for (auto it = PartTransitions.begin(); it != PartTransitions.end();)
		{
			if (std::ranges::find(visited, it->first) == visited.end())
			{
				it = PartTransitions.erase(it);
			}
			else
			{
				++it;
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
		node->A11y = element.A11y;
		node->Parts = element.Parts;

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

	auto computeNodeSpace(const Node& node, const Rectangle& box) -> NodeSpace
	{
		NodeSpace space{};

		if (node.IsBox())
		{
			auto overflow = node.Style.Get<styles::StyleProperty::Overflow>();
			space.Clips = overflow == styles::OverflowType::Hidden ||
						  overflow == styles::OverflowType::Scroll;
			space.Scrolls = overflow == styles::OverflowType::Scroll;
		}

		space.NodeTransform = ComputeNodeTransform(node, box);

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
		gfx::Canvas& canvas,
		const styles::BoxVisual& deco,
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
		gfx::Canvas& canvas,
		const styles::BoxVisual& deco,
		const Rectangle& rect,
		bool squareAsRect = false
	)
	{
		RRect outer{
			.Rect = rect,
			.Radii = deco.Radius,
		};

		if (deco.HasFill)
		{
			if (squareAsRect && !deco.Radius.HasRadius())
			{
				canvas.DrawRect(rect, deco.Fill);
			}
			else
			{
				canvas.DrawRRect(outer, deco.Fill);
			}
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

	auto Node::ResolveDecoration(float w, float h) -> const styles::BoxVisual*
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
		styles::BoxVisual deco = styles::resolveBoxVisual(Style, w, h);

		Deco = deco.IsEmpty()
				   ? nullptr
				   : std::make_unique<styles::BoxVisual>(deco);
		DecoWidth = w;
		DecoHeight = h;
		DecoDirty = false;
	}

	struct PartRefs
	{
		Rectangle HostBox{};
		Rectangle ContentBox{};
		const mtext::TextLayout* Layout = nullptr;
		Rectangle TextOrigin{};
		styles::TextAlignment Align = styles::TextAlignment::Start;
	};

	auto contentExtentRect(const Node& node, const Rectangle& box) -> Rectangle
	{
		if (!node.IsBox() ||
			node.Style.Get<styles::StyleProperty::Overflow>() !=
				styles::OverflowType::Scroll)
		{
			return box;
		}

		float maxX = 0.0F;
		float maxY = 0.0F;
		contentExtent(&node, maxX, maxY);

		return {
			.X = box.X,
			.Y = box.Y,
			.Width = std::max(box.Width, maxX - node.GetX()),
			.Height = std::max(box.Height, maxY - node.GetY()),
		};
	}

	using PartTransitionMap =
		std::unordered_map<std::uint64_t, PartTransitionState>;

	auto cachedPartStyle(const PartTransitionMap* states, std::uint64_t key)
		-> const styles::ComputedStyle*
	{
		if (states == nullptr)
		{
			return nullptr;
		}

		auto it = states->find(key);
		if (it == states->end() || !it->second.Initialized)
		{
			return nullptr;
		}

		return &it->second.Current;
	}

	struct PartBox
	{
		Rectangle Border{};
		Rectangle Content{};
	};

	auto partAnimating(const PartTransitionMap* states, std::uint64_t key)
		-> bool
	{
		if (states == nullptr)
		{
			return false;
		}

		auto it = states->find(key);
		return it != states->end() && !it->second.Transitions.empty();
	}

	auto partEdgeSize(
		const styles::Length& padding,
		const styles::Length& border,
		float basis,
		float emBasis
	) -> float
	{
		return detail::resolveLength(padding, basis, emBasis) +
			   detail::resolveLength(border, basis, emBasis);
	}

	auto resolvePartBox(
		const styles::ComputedStyle& style,
		const Rectangle& reference,
		float emBasis,
		bool allowOvershoot
	) -> PartBox
	{
		auto x = PartAxis::Resolve(
			detail::specifiedLength(style.LayoutPosition.Left),
			detail::specifiedLength(style.LayoutPosition.Right),
			detail::specifiedLength(style.LayoutSizing.Width),
			reference.X,
			reference.Width,
			reference.Width,
			emBasis,
			style.LayoutSpacing.MarginLeft,
			style.LayoutSpacing.MarginRight,
			allowOvershoot
		);

		auto y = PartAxis::Resolve(
			detail::specifiedLength(style.LayoutPosition.Top),
			detail::specifiedLength(style.LayoutPosition.Bottom),
			detail::specifiedLength(style.LayoutSizing.Height),
			reference.Y,
			reference.Height,
			reference.Height,
			emBasis,
			style.LayoutSpacing.MarginTop,
			style.LayoutSpacing.MarginBottom,
			allowOvershoot
		);

		Rectangle border{
			.X = x.Pos,
			.Y = y.Pos,
			.Width = x.Size,
			.Height = y.Size,
		};

		float left = partEdgeSize(
			style.LayoutSpacing.PaddingLeft,
			style.LayoutSpacing.BorderLeft,
			reference.Width,
			emBasis
		);
		float right = partEdgeSize(
			style.LayoutSpacing.PaddingRight,
			style.LayoutSpacing.BorderRight,
			reference.Width,
			emBasis
		);
		float top = partEdgeSize(
			style.LayoutSpacing.PaddingTop,
			style.LayoutSpacing.BorderTop,
			reference.Height,
			emBasis
		);
		float bottom = partEdgeSize(
			style.LayoutSpacing.PaddingBottom,
			style.LayoutSpacing.BorderBottom,
			reference.Height,
			emBasis
		);

		return {
			.Border = border,
			.Content =
				{
					.X = border.X + left,
					.Y = border.Y + top,
					.Width = std::max(0.0F, border.Width - left - right),
					.Height = std::max(0.0F, border.Height - top - bottom),
				},
		};
	}

	void paintPart(
		gfx::Canvas& canvas,
		const Part& part,
		std::size_t siblingIndex,
		std::uint64_t parentKey,
		const PartTransitionMap* states,
		const styles::ComputedStyle& parentStyle,
		const Rectangle& reference,
		const Rectangle& hostBox
	)
	{
		styles::ComputedStyle resolved;
		std::uint64_t key = partKey(parentKey, part, siblingIndex);
		const styles::ComputedStyle* style = cachedPartStyle(states, key);

		if (style == nullptr)
		{
			resolved = styles::ComputedStyle::Cascade(
				part.Style.Entries(),
				parentStyle
			);
			style = &resolved;
		}

		float emBasis = style->Inherited.FontSize.Value;

		PartBox layout = resolvePartBox(
			*style,
			reference,
			emBasis,
			partAnimating(states, key)
		);
		Rectangle rect = layout.Border;

		if (part.Anchor.Tag == PartAnchor::Kind::Caret)
		{
			rect.Width = std::max(rect.Width, 1.0F);
			rect.Height = std::max(rect.Height, 1.0F);
			rect.X = std::clamp(
				rect.X,
				hostBox.X,
				hostBox.X + hostBox.Width - rect.Width
			);
			rect.Y = std::clamp(
				rect.Y,
				hostBox.Y,
				hostBox.Y + hostBox.Height - rect.Height
			);
		}

		for (std::size_t i = 0; i < part.Parts.size(); ++i)
		{
			const Part& nested = part.Parts[i];
			if (nested.Layer == PartLayer::Behind)
			{
				paintPart(
					canvas,
					nested,
					i,
					key,
					states,
					*style,
					layout.Content,
					hostBox
				);
			}
		}

		if (!rect.IsEmpty())
		{
			styles::BoxVisual visual =
				styles::resolveBoxVisual(*style, rect.Width, rect.Height);

			if (!visual.IsEmpty())
			{
				paintDecoration(canvas, visual, rect, true);
			}
		}

		for (std::size_t i = 0; i < part.Parts.size(); ++i)
		{
			const Part& nested = part.Parts[i];
			if (nested.Layer == PartLayer::Above)
			{
				paintPart(
					canvas,
					nested,
					i,
					key,
					states,
					*style,
					layout.Content,
					hostBox
				);
			}
		}
	}

	void paintParts(
		gfx::Canvas& canvas,
		std::span<const Part> parts,
		PartLayer layer,
		PartSpace space,
		const styles::ComputedStyle& parentStyle,
		const PartRefs& refs,
		const PartTransitionMap* states,
		std::uint64_t hostKey
	)
	{
		for (std::size_t i = 0; i < parts.size(); ++i)
		{
			const Part& part = parts[i];

			if (part.Layer != layer)
			{
				continue;
			}

			auto effectiveSpace =
				part.Anchor.Tag == PartAnchor::Kind::Box ? part.Space : PartSpace::Content;
			if (effectiveSpace != space)
			{
				continue;
			}

			switch (part.Anchor.Tag)
			{
			case PartAnchor::Kind::Box:
				paintPart(
					canvas,
					part,
					i,
					hostKey,
					states,
					parentStyle,
					space == PartSpace::Content ? refs.ContentBox
												: refs.HostBox,
					refs.HostBox
				);
				break;
			case PartAnchor::Kind::Range:
			{
				mc_assert(
					refs.Layout != nullptr,
					"a Range part needs a text host"
				);
				if (refs.Layout == nullptr)
				{
					break;
				}

				for (const auto& span : mtext::TextGeometry::SpanBoxes(
						 *refs.Layout,
						 part.Anchor.Range.Start,
						 part.Anchor.Range.End,
						 refs.TextOrigin.Width,
						 refs.Align
					 ))
				{
					paintPart(
						canvas,
						part,
						i,
						hostKey,
						states,
						parentStyle,
						{
							.X = refs.TextOrigin.X + span.X,
							.Y = refs.TextOrigin.Y + span.Top,
							.Width = span.Width,
							.Height = span.Height,
						},
						refs.HostBox
					);
				}
				break;
			}
			case PartAnchor::Kind::Caret:
			{
				mc_assert(
					refs.Layout != nullptr,
					"a Caret part needs a text host"
				);
				if (refs.Layout == nullptr)
				{
					break;
				}

				auto caret = mtext::TextGeometry::CaretAt(
					*refs.Layout,
					{
						.Offset = part.Anchor.Range.Start,
						.Affinity = part.Anchor.Affinity,
					},
					refs.TextOrigin.Width,
					refs.Align
				);

				paintPart(
					canvas,
					part,
					i,
					hostKey,
					states,
					parentStyle,
					{
						.X = std::round(refs.TextOrigin.X + caret.X),
						.Y = std::round(refs.TextOrigin.Y + caret.Y),
						.Width = 0.0F,
						.Height = std::round(caret.Height),
					},
					refs.HostBox
				);
				break;
			}
			}
		}
	}

	void paintTextContent(
		Node& node,
		gfx::Canvas& canvas,
		const Rectangle& contentBox,
		const std::shared_ptr<const mtext::TextLayout>& layout
	)
	{
		mc_profileFunction();

		const auto& textElement = std::get<TextElement>(node.Kind);

		canvas.DrawText(
			gfx::cmds::DrawTextCmd{
				.Position = {.X = contentBox.X, .Y = contentBox.Y},
				.Content = textElement.Content,
				.Paint = gfx::Paint::Fill(
					node.Style.Get<styles::StyleProperty::TextColor>()
				),
				.Size = node.Style.Get<styles::StyleProperty::FontSize>().Value,
				.Face = layout->Face,
				.Layout = layout,
				.BoxWidth = contentBox.Width,
				.Align = node.Style.Get<styles::StyleProperty::TextAlign>(),
			}
		);
	}

	void paintNode(
		Node& node,
		gfx::Canvas& canvas,
		const Rectangle& cull,
		float x,
		float y,
		float w,
		float h
	)
	{
		mc_profileFunction();

		Rectangle box{.X = x, .Y = y, .Width = w, .Height = h};
		NodeSpace space = computeNodeSpace(node, box);

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

		PartRefs refs{};
		refs.HostBox = box;
		refs.ContentBox = contentExtentRect(node, box);

		std::shared_ptr<const mtext::TextLayout> textLayout;
		if (node.IsText())
		{
			refs.TextOrigin = contentRect(node, box);
			refs.Align = node.Style.Get<styles::StyleProperty::TextAlign>();
			textLayout = mtext::detail::TextLayoutBuilder::ResolveTextLayout(
				node.Style,
				std::get<TextElement>(node.Kind).Content,
				refs.TextOrigin.Width,
				true
			);
			refs.Layout = textLayout.get();
		}

		if (node.Parts != nullptr)
		{
			paintParts(
				canvas,
				*node.Parts,
				PartLayer::Behind,
				PartSpace::Host,
				node.Style,
				refs,
				&node.PartTransitions,
				static_cast<std::uint64_t>(node.Id)
			);
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

		if (node.Parts != nullptr)
		{
			paintParts(
				canvas,
				*node.Parts,
				PartLayer::Behind,
				PartSpace::Content,
				node.Style,
				refs,
				&node.PartTransitions,
				static_cast<std::uint64_t>(node.Id)
			);
		}

		if (node.IsText())
		{
			paintTextContent(node, canvas, refs.TextOrigin, textLayout);
		}

		Rectangle childCull =
			narrowCull(cull, space, box, node.ScrollOffset);
		for (auto& child : node.Children)
		{
			child->Paint(canvas, childCull);
		}

		if (node.Parts != nullptr)
		{
			paintParts(
				canvas,
				*node.Parts,
				PartLayer::Above,
				PartSpace::Content,
				node.Style,
				refs,
				&node.PartTransitions,
				static_cast<std::uint64_t>(node.Id)
			);
		}

		if (scoped)
		{
			canvas.Restore();
		}

		if (node.Parts != nullptr)
		{
			paintParts(
				canvas,
				*node.Parts,
				PartLayer::Above,
				PartSpace::Host,
				node.Style,
				refs,
				&node.PartTransitions,
				static_cast<std::uint64_t>(node.Id)
			);
		}

		if (space.HasTransform)
		{
			canvas.Restore();
		}
	}

	void Node::Paint(gfx::Canvas& canvas, const Rectangle& cull)
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

			if (current.Content != incoming.Content)
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
		oldNode->Focus = newElement->Focus;
		oldNode->Parts = newElement->Parts;
		if (!sameA11y(oldNode->A11y, newElement->A11y))
		{
			oldNode->A11y = newElement->A11y;
			if (auto* surface = getCtx()->_currentSurface; surface != nullptr)
			{
				surface->MarkA11yDirty();
			}
		}
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

		NodeSpace space = computeNodeSpace(*node, box);
		if (!space.Invertible)
		{
			return nullptr;
		}

		Vector2 localPoint = space.ParentToLocal.MapPoint(parentPoint);
		bool inside = !node->IsComponent() && box.Contains(localPoint);

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

	auto Node::FindNodeById(Node* root, NodeId id) -> Node*
	{
		if (getCtx() == nullptr)
		{
			return nullptr;
		}

		return getCtx()->FindNodeById(id);
	}
}