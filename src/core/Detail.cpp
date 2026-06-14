#include "Detail.h"
#include <cstddef>
#include <iostream>

namespace mocca::detail
{
	static NodeId nextNodeId = 0;
	auto Node::BuildNodeTree(const Element& element) -> std::unique_ptr<Node>
	{
		auto node = std::make_unique<Node>();
		node->Id = nextNodeId++;
		node->Key = element.Key;

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
					node->Kind = TextNode{.Content = arm.Content};
				}
				else if constexpr (std::is_same_v<T, ComponentElement>)
				{
					node->Kind = ComponentNode{.Fn = arm.Fn};

					// run it
					auto produced = arm.Fn();
					auto childNode = BuildNodeTree(produced);
					childNode->Parent = node.get();
					node->Children.push_back(std::move(childNode));
				}
			},
			element.Node);

		return node;
	}

	void Node::Print(int depth)
	{
		std::string indent(static_cast<size_t>(depth * 2), ' ');
		auto kind = NodeKind();

		std::cout << indent << "<" << kind << " id=\"" << Id << "\"";

		if (Key != mc_keyNone)
		{
			std::cout << " key=\"" << Key << "\"";
		}

		std::cout << ((Children.empty() && kind != "Text") ? "/>" : ">")
				  << '\n';

		if (IsText())
		{
			const auto& textNode = std::get<TextNode>(Kind);
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

	auto Node::Reconcile(std::unique_ptr<Node> oldNode,
						 const Element* newElement) -> std::unique_ptr<Node>
	{
		if (newElement == nullptr)
		{
			return nullptr;
		}

		if (oldNode == nullptr)
		{
			oldNode = BuildNodeTree(*newElement);
		}
		else if (oldNode->Kind.index() != newElement->Node.index() ||
			oldNode->Key != newElement->Key)
		{
			// zombie instead of killing old tree - later
			oldNode = BuildNodeTree(*newElement);
		}

		// props here
		if (oldNode->IsText())
        {
            auto& textNode = std::get<TextNode>(oldNode->Kind);
            const auto& textElement = std::get<TextElement>(newElement->Node);
            textNode.Content = textElement.Content;
        }

		std::vector<Element> childElements;

		if (std::holds_alternative<BoxElement>(newElement->Node))
		{
			const auto& box = std::get<BoxElement>(newElement->Node);
			childElements = box.Children;
		}
		else if (std::holds_alternative<ComponentElement>(newElement->Node))
		{
			const auto& component =
				std::get<ComponentElement>(newElement->Node);
			auto produced = component.Fn();
			childElements.push_back(produced);
		}

		size_t maxCount =
			std::max(oldNode->Children.size(), childElements.size());

		for (size_t i = 0; i < maxCount; i++)
		{
			std::unique_ptr<Node> oldChild;
            if (i < oldNode->Children.size())
            {
                oldChild = std::move(oldNode->Children[i]);
            }

            const Element* newChild = nullptr;
            if (i < childElements.size())
            {
                newChild = &childElements[i];
            }

            auto reconciledChild = Reconcile(std::move(oldChild), newChild);
            if (reconciledChild != nullptr)
            {
                reconciledChild->Parent = oldNode.get();
            }

            if (i < oldNode->Children.size())
            {
                oldNode->Children[i] = std::move(reconciledChild);
            }
            else if (reconciledChild != nullptr)
            {
                oldNode->Children.push_back(std::move(reconciledChild));
            }
		}

		return oldNode;
	}
}