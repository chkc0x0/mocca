#pragma once

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
}