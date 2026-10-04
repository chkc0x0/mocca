#pragma once
#include <vector>
#include "core/Node.h"

namespace mocca::detail
{
	template <typename Fn>
	auto dispatchPointerEvent(
		Node* root,
		NodeId capturedNode,
		PointerEvent& ev,
		Fn callback
	) -> Node*
	{
		Node* target = nullptr;
		if (capturedNode != 0)
		{
			target = Node::FindNodeById(root, capturedNode);
		}
		if (target == nullptr)
		{
			target = Node::HitTest(root, ev.X, ev.Y);
		}
		if (target == nullptr)
		{
			return nullptr;
		}
		std::vector<Node*> path;
		for (Node* n = target; n != nullptr; n = n->Parent)
		{
			path.push_back(n);
		}
		for (Node* n : path)
		{
			if (ev.StopPropagation)
			{
				break;
			}
			callback(n, ev);
		}

		return target;
	}

	template <typename Fn, typename Ev>
	void
	dispatchKeyTextEvent(Node* root, NodeId focusedNode, Ev& ev, Fn callback)
	{
		Node* target = nullptr;
		if (focusedNode != 0)
		{
			target = Node::FindNodeById(root, focusedNode);
		}
		if (target == nullptr)
		{
			return;
		}
		std::vector<Node*> path;
		for (Node* n = target; n != nullptr; n = n->Parent)
		{
			path.push_back(n);
		}
		for (Node* n : path)
		{
			if (ev.StopPropagation)
			{
				break;
			}
			callback(n, ev);
		}
	}
}