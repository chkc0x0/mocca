#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <variant>
#include <vector>

#include "Element.h"

namespace mocca::detail
{
	using NodeId = std::uint64_t;

	struct TextNode
	{
		std::string Content;
	};

	struct ComponentNode
	{
		ComponentFn Fn;
	};

	struct Node
	{
		NodeId Id;
		ElementKey Key = mc_keyNone;

		Node* Parent = nullptr; // not read yet

		std::vector<std::unique_ptr<Node>> Children;

		std::variant<std::monostate, TextNode, ComponentNode> Kind;

		[[nodiscard]] auto IsBox() const -> bool
		{
			return std::holds_alternative<std::monostate>(Kind);
		}
		[[nodiscard]] auto IsText() const -> bool
		{
			return std::holds_alternative<TextNode>(Kind);
		}
		[[nodiscard]] auto IsComponent() const -> bool
		{
			return std::holds_alternative<ComponentNode>(Kind);
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

		static auto BuildNodeTree(const Element& element)
			-> std::unique_ptr<Node>;

		static auto Reconcile(std::unique_ptr<Node> oldNode,
							  const Element* newElement)
			-> std::unique_ptr<Node>;

		void Print(int depth = 0);
	};

}