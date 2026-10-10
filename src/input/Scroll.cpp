#include "input/Scroll.h"
#include "core/Context.h"
#include "core/Application.h"
#include "core/Node.h"
#include "style/ComputedStyle.h"
#include "yoga/YGNodeLayout.h"

namespace mocca::detail
{
	auto findScrollableAncestor(Node* node) -> Node*
	{
		for (Node* n = node; n != nullptr; n = n->Parent)
		{
			if (n->IsBox() && n->Style.Get<styles::StyleProperty::Overflow>() == OverflowType::Scroll)
			{
				return n;
			}
		}
		return nullptr;
	}

	auto contentExtent(Node* n, float& maxX, float& maxY) -> void
	{
		if (n == nullptr)
		{
			return;
		}

		if (!n->IsComponent() && n->YogaNode != nullptr)
		{
			maxX =
				std::max(maxX, n->GetX() + YGNodeLayoutGetWidth(n->YogaNode));
			maxY =
				std::max(maxY, n->GetY() + YGNodeLayoutGetHeight(n->YogaNode));
		}

		for (auto& c : n->Children)
		{
			contentExtent(c.get(), maxX, maxY);
		}
	}

	auto reclampScrollOffsets(Node* n) -> void
	{
		if (n == nullptr)
		{
			return;
		}

		if (n->IsBox() &&
			n->Style.Get<styles::StyleProperty::Overflow>() ==
				OverflowType::Scroll &&
			n->YogaNode != nullptr)
		{
			float maxX = 0.0F;
			float maxY = 0.0F;
			contentExtent(n, maxX, maxY);

			float vx = n->GetX();
			float vy = n->GetY();
			float vw = YGNodeLayoutGetWidth(n->YogaNode);
			float vh = YGNodeLayoutGetHeight(n->YogaNode);

			float maxSx = std::max(0.0F, maxX - (vx + vw));
			float maxSy = std::max(0.0F, maxY - (vy + vh));

			n->ScrollOffset.X = std::clamp(n->ScrollOffset.X, 0.0F, maxSx);
			n->ScrollOffset.Y = std::clamp(n->ScrollOffset.Y, 0.0F, maxSy);
		}

		for (auto& c : n->Children)
		{
			reclampScrollOffsets(c.get());
		}
	}
}