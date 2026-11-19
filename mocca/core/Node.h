#pragma once
#include "Element.h"
#include "gfx/Canvas.h"
#include "yoga/YGNode.h"
#include "style/ComputedStyle.h"
#include "anim/Curve.h"
#include "anim/Spring.h"
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <vector>

namespace mocca
{
	class Surface;

	namespace styles
	{
		struct BoxVisual;
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

	struct PartTransitionState
	{
	public:
		styles::StyleId Current = styles::DefaultStyleId;
		std::vector<ActiveTransition> Transitions;
		bool Initialized = false;
	};

	struct Node
	{
		Node();
		~Node();

		NodeId Id;
		ElementKey Key = mc_keyNone;

		Node* Parent = nullptr;
		class Surface* Surface = nullptr;
		YGNodeRef YogaNode{nullptr};

		styles::StyleSet Declared;
		styles::StyleId Style = styles::DefaultStyleId;
		std::vector<ActiveTransition> Transitions;

		bool Styled = false;
		uint8_t DepGroups = styles::InheritedGroupBit;
		uint8_t ChangedGroups = 0;
		bool StyleDirty = false;
		bool YogaDirty = true;
		bool DecoDirty = true;
		FocusMode Focus = FocusMode::None;

		std::unique_ptr<styles::BoxVisual> Deco;
		float DecoWidth = -1.0F;
		float DecoHeight = -1.0F;

		Vector2 ScrollOffset{.X = 0, .Y = 0};
		Vector2 LayoutPos{.X = 0, .Y = 0};
		Rectangle SubtreeBounds{};
		std::vector<std::unique_ptr<Node>> Children;
		std::variant<std::monostate, TextElement, ComponentElement> Kind;

		std::shared_ptr<EventView> Events;
		aria::A11y A11y = nullptr;
		PartList Parts;
		std::unordered_map<std::uint64_t, PartTransitionState> PartTransitions;

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
			const styles::ComputedStyle& parentComputed,
			double dt,
			uint8_t parentChangedGroups
		);
		void AdvanceTransitions(
			styles::ComputedStyle& current,
			styles::ComputedStyle& goal,
			double dt
		);
		void AdvancePartTransitions(double dt, bool& anyActive);
		auto FindTransition(styles::StyleProperty prop) -> ActiveTransition*;
		void RemoveTransition(styles::StyleProperty prop);

		[[nodiscard]] auto ResolveStyle() const
			-> const styles::ComputedStyle&;

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
		void Paint(gfx::Canvas& canvas, const Rectangle& cull);
		auto ResolveDecoration(float w, float h) -> const styles::BoxVisual*;

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