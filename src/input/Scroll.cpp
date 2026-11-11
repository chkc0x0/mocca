#include "input/Scroll.h"
#include "core/Context.h"
#include "core/Node.h"
#include "core/Surface.h"
#include "style/ComputedStyle.h"
#include "utils/ScopedProfiler.h"
#include "yoga/YGNodeLayout.h"
#include <algorithm>
#include <cmath>
#include <vector>

namespace mocca::detail
{
	auto findScrollableAncestor(Node* node) -> Node*
	{
		mc_profileFunction();
		
		for (Node* n = node; n != nullptr; n = n->Parent)
		{
			if (n->IsBox() && n->Style.Get<styles::StyleProperty::Overflow>() ==
								  styles::OverflowType::Scroll)
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

	auto paintedRect(const Node& node) -> Rectangle
	{
		Rectangle rect{
			.X = node.GetX(),
			.Y = node.GetY(),
			.Width = YGNodeLayoutGetWidth(node.YogaNode),
			.Height = YGNodeLayoutGetHeight(node.YogaNode),
		};

		for (auto* parent = node.Parent; parent != nullptr;
			 parent = parent->Parent)
		{
			if (parent->IsBox() &&
				parent->Style.Get<styles::StyleProperty::Overflow>() ==
					styles::OverflowType::Scroll)
			{
				rect.X -= parent->ScrollOffset.X;
				rect.Y -= parent->ScrollOffset.Y;
			}
		}

		return rect;
	}

	void scrollRectIntoView(Node* node, Surface* surface, Rectangle target)
	{
		if (node == nullptr || surface == nullptr)
		{
			return;
		}

		std::vector<Node*> scrollables;
		for (auto* parent = node->Parent; parent != nullptr;
			 parent = parent->Parent)
		{
			if (parent->IsBox() &&
				parent->Style.Get<styles::StyleProperty::Overflow>() ==
					styles::OverflowType::Scroll)
			{
				scrollables.push_back(parent);
			}
		}

		bool changed = false;
		for (auto* scrollable : scrollables)
		{
			Rectangle viewport = paintedRect(*scrollable);

			float contentMaxX = 0.0F;
			float contentMaxY = 0.0F;
			contentExtent(scrollable, contentMaxX, contentMaxY);

			float maxSx = std::max<float>(
				0.0F,
				contentMaxX - (scrollable->GetX() +
							   YGNodeLayoutGetWidth(scrollable->YogaNode))
			);
			float maxSy = std::max<float>(
				0.0F,
				contentMaxY - (scrollable->GetY() +
							   YGNodeLayoutGetHeight(scrollable->YogaNode))
			);

			float sx = scrollable->ScrollOffset.X;
			float sy = scrollable->ScrollOffset.Y;

			float cx = target.X - viewport.X + sx;
			float cy = target.Y - viewport.Y + sy;

			if (cx < sx)
			{
				sx = cx;
			}
			else if (cx + target.Width > sx + viewport.Width)
			{
				sx = cx + target.Width - viewport.Width;
			}

			if (cy < sy)
			{
				sy = cy;
			}
			else if (cy + target.Height > sy + viewport.Height)
			{
				sy = cy + target.Height - viewport.Height;
			}

			sx = std::round(std::clamp(sx, 0.0F, maxSx));
			sy = std::round(std::clamp(sy, 0.0F, maxSy));

			float dx = sx - scrollable->ScrollOffset.X;
			float dy = sy - scrollable->ScrollOffset.Y;

			if (dx == 0.0F && dy == 0.0F)
			{
				continue;
			}

			scrollable->ScrollOffset.X = sx;
			scrollable->ScrollOffset.Y = sy;
			target.X -= dx;
			target.Y -= dy;
			changed = true;
		}

		if (changed)
		{
			surface->MarkDirty();
			surface->MarkA11yDirty();
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
				styles::OverflowType::Scroll &&
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