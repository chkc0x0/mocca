#pragma once
#include "utils/Math.h"

namespace mocca
{
	class Surface;
}

namespace mocca::detail
{
	struct Node;
	
	auto findScrollableAncestor(Node* node) -> Node*;

	auto contentExtent(Node* n, float& maxX, float& maxY) -> void;
	auto reclampScrollOffsets(Node* n) -> void;

	auto paintedRect(const Node& node) -> Rectangle;
	void scrollRectIntoView(Node* node, Surface* surface, Rectangle target);
}