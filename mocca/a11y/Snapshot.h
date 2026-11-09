#pragma once
#include "a11y/A11y.h"
#include "core/Identifiers.h"
#include "input/InputTypes.h"
#include "utils/Math.h"
#include <cstddef>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace mocca::aria
{
	struct Node
	{
		detail::NodeId Id = 0;
		mocca::aria::Role Role = mocca::aria::Role::None;
		std::string Name;
		std::string TextValue;
		Rectangle Bounds{};
		std::vector<detail::NodeId> Children;
		A11y Props = nullptr;
		std::vector<detail::NodeId> LabelledBy;
		std::vector<detail::NodeId> DescribedBy;
		std::vector<detail::NodeId> Controls;
		std::vector<detail::NodeId> Details;
		std::vector<detail::NodeId> Owns;
		std::vector<detail::NodeId> FlowTo;
	};

	struct Snapshot
	{
		detail::NodeId Root = 0;
		detail::NodeId Focus = 0;
		std::string Title;
		float Scale = 1.0F;
		std::vector<Node> Nodes;
		std::unordered_map<detail::NodeId, size_t> Index;

		[[nodiscard]] auto Find(detail::NodeId id) const -> const Node*;
	};

	struct ActionRequest
	{
		detail::NodeId Target = 0;
		A11yAction Action = A11yAction::Click;
		std::string Value;
		double NumericValue = 0.0;
	};

	struct RootBounds
	{
		Rectangle Outer{};
		Rectangle Inner{};
	};

	class Provider
	{
	public:
		virtual ~Provider() = default;

		virtual auto A11ySnapshot() -> std::shared_ptr<const Snapshot> = 0;
		virtual void QueueA11yAction(ActionRequest req) = 0;
	};

	class Bridge
	{
	public:
		virtual ~Bridge() = default;

		virtual void UpdateIfActive() = 0;
		virtual void SetWindowFocused(bool focused) = 0;
		virtual void SetRootBounds(const RootBounds& bounds) = 0;
		virtual void SetScale(float scale) = 0;
		[[nodiscard]] virtual auto Active() const -> bool = 0;
	};
}
