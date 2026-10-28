#pragma once
#include "Element.h"
#include "gfx/Canvas.h"
#include "style/StyleBuilder.h"
#include "yoga/YGNode.h"
#include "style/ComputedStyle.h"
#include "anim/Curve.h"
#include "anim/Spring.h"
#include <algorithm>
#include <memory>
#include <vector>

namespace mocca::detail
{
	inline NodeId nextNodeId = 1;

	struct ActiveTransition
	{
	public:
		styles::StyleProperty Prop = styles::StyleProperty::None;
		styles::StyleEntry From;
		styles::StyleEntry Goal;
		float Elapsed = 0.0F;
		float Duration = 0.0F;
		float Progress = 0.0F;
		anim::Curve Curve{};
		anim::SpringState Spring{};
	};

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
		std::vector<ActiveTransition> Transitions;
		bool Styled = false;

		Vector2 ScrollOffset{.X = 0, .Y = 0};
		std::vector<std::unique_ptr<Node>> Children;
		std::variant<std::monostate, TextElement, ComponentElement> Kind;

		std::shared_ptr<EventView> Events;
		FocusMode Focus = FocusMode::None;
		CursorShape Cursor = CursorShape::Default;

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
		void ComputeStyle(styles::ComputedStyle& parentComputed, double dt);
		void AdvanceTransitions(styles::ComputedStyle& target, double dt);
		auto FindTransition(styles::StyleProperty prop) -> ActiveTransition*;
		void RemoveTransition(styles::StyleProperty prop);

		template <styles::StyleProperty P>
		void StepTransition(
			styles::ComputedStyle& target,
			double dt,
			bool& anyActive
		)
		{
			if (target.Get<styles::StyleProperty::Transition>() == 0)
			{
				return;
			}
			
			using Value = styles::PropTraits<P>::Value;

			const styles::TransitionType& trans =
				styles::detail::resolveTransition(
					target.Get<styles::StyleProperty::Transition>()
				);

			if ((trans.Props & styles::propertyBit<P>()) == 0)
			{
				RemoveTransition(P);
				return;
			}

			Value goal = target.Get<P>();
			ActiveTransition* tr = FindTransition(P);

			if (tr == nullptr)
			{
				Value present = Style.Get<P>();
				if (present == goal)
				{
					return;
				}

				ActiveTransition fresh;
				fresh.Prop = P;
				fresh.From = styles::detail::encodeStyleEntry(P, present);
				fresh.Goal = styles::detail::encodeStyleEntry(P, goal);
				fresh.Duration = trans.Duration;
				fresh.Curve = trans.Curve;
				Transitions.push_back(fresh);
				tr = &Transitions.back();
			}
			else if (styles::getStyleEntryValue<Value>(tr->Goal) != goal)
			{
				tr->From = styles::detail::encodeStyleEntry(P, Style.Get<P>());
				tr->Goal = styles::detail::encodeStyleEntry(P, goal);
				tr->Elapsed = 0.0F;
				tr->Progress = 0.0F;
				tr->Spring = {};
				tr->Duration = trans.Duration;
				tr->Curve = trans.Curve;
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
				RemoveTransition(P);
			}
			else
			{
				anyActive = true;
			}
		}

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