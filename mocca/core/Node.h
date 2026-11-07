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

namespace mocca
{
	class Surface;

	namespace gfx
	{
		struct BoxDecoration;
	}
}

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
		Surface* Surface = nullptr;
		YGNodeRef YogaNode{nullptr};

		styles::StyleSet Declared;
		styles::ComputedStyle Style;
		std::vector<ActiveTransition> Transitions;
		bool Styled = false;

		uint8_t DepGroups = styles::InheritedGroupBit;
		uint8_t ChangedGroups = 0;
		bool StyleDirty = false;
		bool YogaDirty = true;

		std::unique_ptr<gfx::BoxDecoration> Deco;
		float DecoWidth = -1.0F;
		float DecoHeight = -1.0F;
		bool DecoDirty = true;

		Vector2 ScrollOffset{.X = 0, .Y = 0};
		Vector2 LayoutPos{.X = 0, .Y = 0};
		Rectangle SubtreeBounds{};
		std::vector<std::unique_ptr<Node>> Children;
		std::variant<std::monostate, TextElement, ComponentElement> Kind;

		std::shared_ptr<EventView> Events;
		FocusMode Focus = FocusMode::None;

		uint64_t BlinkToken = 0;
		double BlinkEpochMs = 0.0;

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
		void UpdateSubtreeBounds();
		void ApplyLayoutStyles() const;
		void ComputeStyle(
			styles::ComputedStyle& parentComputed,
			double dt,
			uint8_t parentChangedGroups
		);
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
			uint32_t transitionRef =
				target.Get<styles::StyleProperty::Transition>();
			if (transitionRef == 0)
			{
				RemoveTransition(P);
				return;
			}

			using Value = styles::PropTraits<P>::Value;

			const styles::TransitionList& transitions =
				styles::detail::resolveTransition(transitionRef);

			const styles::TransitionType* spec = nullptr;
			for (auto it = transitions.Specs.rbegin();
				 it != transitions.Specs.rend();
				 ++it)
			{
				if ((it->Props & styles::propertyBit<P>()) != 0)
				{
					spec = &*it;
					break;
				}
			}

			if (spec == nullptr)
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
				fresh.Duration = spec->Duration;
				fresh.Curve = spec->Curve;
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
				RemoveTransition(P);
			}
			else
			{
				anyActive = true;
			}
		}

		[[nodiscard]] auto GetX() const -> float;
		[[nodiscard]] auto GetY() const -> float;

		static auto ReconcileChildren(
			std::vector<std::unique_ptr<Node>> old,
			std::span<const Element> kids,
			Node* parent
		) -> std::vector<std::unique_ptr<Node>>;

		static auto Reconcile(
			std::unique_ptr<Node> oldNode,
			const Element* newElement
		) -> std::unique_ptr<Node>;

		void Print(int depth = 0) const;
		void Paint(Canvas& canvas, const Rectangle& cull);
		auto ResolveDecoration(float w, float h) -> const gfx::BoxDecoration*;

		static auto HitTest(Node* root, float x, float y) -> Node*;
		static auto FindNodeById(Node* root, NodeId id) -> Node*;

	private:
		void _resolveDecoration(float w, float h);

		static auto ReconcileChildrenSlow(
			std::vector<std::unique_ptr<Node>> oldChildren,
			std::span<const Element> childElements,
			Node* parent
		) -> std::vector<std::unique_ptr<Node>>;
	};
}