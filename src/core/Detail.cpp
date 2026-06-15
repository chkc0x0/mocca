#include "Detail.h"
#include "Context.h"
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
					node->Kind = TextElement{.Content = arm.Content};
				}
				else if constexpr (std::is_same_v<T, ComponentElement>)
				{
					node->Kind = ComponentElement{.Fn = arm.Fn};
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

	auto Node::Reconcile(std::unique_ptr<Node> oldNode,
						 const Element* newElement) -> std::unique_ptr<Node>
	{
		if (newElement == nullptr && oldNode == nullptr)
		{
			return nullptr;
		}

		if (newElement == nullptr && oldNode != nullptr)
		{
			getCtx()->_store.RemoveComponent(oldNode->Id);
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
			getCtx()->_store.RemoveComponent(oldNode->Id);
			oldNode = BuildNodeTree(*newElement);
		}

		if (oldNode->IsText())
		{
			oldNode->Kind = std::get<TextElement>(newElement->Node);
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

			// set up ctx
			auto produced = Element::Render(component.Fn, component.Props, oldNode->Id);
			childElements.push_back(produced);
		}

		std::unordered_map<ElementKey, std::unique_ptr<Node>> oldKeyed;
		std::vector<std::unique_ptr<Node>> oldUnkeyed;

		for (auto& child : oldNode->Children)
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

		// match by key
		for (auto& child : childElements)
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
			else
			{
				if (unkeyedCursor < oldUnkeyed.size())
				{
					matchedOld = std::move(oldUnkeyed[unkeyedCursor++]);
				}
			}

			auto reconciled = Reconcile(std::move(matchedOld), &child);
			if (reconciled)
			{
				reconciled->Parent = oldNode.get();
				newChildren.push_back(std::move(reconciled));
			}
		}

		// kill orphans
		for (auto& [k, n] : oldKeyed)
		{
			if (n)
			{
				getCtx()->_store.RemoveComponent(n->Id);
			}
		}

		if (oldUnkeyed.size() > unkeyedCursor)
		{
			for (size_t i = unkeyedCursor; i < oldUnkeyed.size(); i++)
			{
				getCtx()->_store.RemoveComponent(oldUnkeyed[i]->Id);
			}
		}

		oldNode->Children = std::move(newChildren);
		return oldNode;
	}
}

namespace mocca
{
	// for the surface
	auto Element::Render(const ComponentFn& fn, std::uint64_t id) -> Element
	{
		auto prev = getCtx()->_enterComponentRender(id);
		auto produced = fn();
		getCtx()->_exitComponentRender(prev);
		return produced;
	}

	// anywhere else
	auto Element::Render(const ComponentPropsFn& fn, const std::any& props, std::uint64_t id) -> Element
	{
		auto prev = getCtx()->_enterComponentRender(id);
		auto produced = fn(props);
		getCtx()->_exitComponentRender(prev);
		return produced;
	}
}