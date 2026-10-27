#include "core/NodeRef.h"
#include "core/Node.h"
#include "core/Surface.h"
#include "input/Scroll.h"
#include "style/ComputedStyle.h"
#include "yoga/YGNodeLayout.h"
#include <algorithm>
#include <vector>

namespace mocca
{
	auto findNode(detail::NodeId id, Surface* surface) -> detail::Node*
	{
		if (surface == nullptr || id == 0)
		{
			return nullptr;
		}

		return detail::Node::FindNodeById(surface->Tree(), id);
	}

	auto paintedRect(const detail::Node& node) -> Rectangle
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

	auto isScrollable(const detail::Node* node) -> bool
	{
		return node != nullptr && node->IsBox() &&
			   node->Style.Get<styles::StyleProperty::Overflow>() ==
				   styles::OverflowType::Scroll;
	}

	auto NodeRef::Valid() const -> bool
	{
		return findNode(_id, _surface) != nullptr;
	}

	auto NodeRef::Id() const -> detail::NodeId
	{
		return _id;
	}

	auto NodeRef::Surface() -> class Surface*
	{
		return _surface;
	}

	auto NodeRef::Rect() const -> Rectangle
	{
		auto* node = findNode(_id, _surface);
		if (node == nullptr)
		{
			return {};
		}

		return paintedRect(*node);
	}

	auto NodeRef::LayoutRect() const -> Rectangle
	{
		auto* node = findNode(_id, _surface);
		if (node == nullptr)
		{
			return {};
		}

		return {
			.X = node->GetX(),
			.Y = node->GetY(),
			.Width = YGNodeLayoutGetWidth(node->YogaNode),
			.Height = YGNodeLayoutGetHeight(node->YogaNode),
		};
	}

	void NodeRef::ScrollIntoView()
	{
		ScrollRectIntoView(Rect());
	}

	void NodeRef::ScrollRectIntoView(Rectangle target)
	{
		auto* node = findNode(_id, _surface);
		if (node == nullptr)
		{
			return;
		}

		std::vector<detail::Node*> scrollables;
		for (auto* parent = node->Parent; parent != nullptr;
			 parent = parent->Parent)
		{
			if (isScrollable(parent))
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
			detail::contentExtent(scrollable, contentMaxX, contentMaxY);

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

			sx = std::clamp(sx, 0.0F, maxSx);
			sy = std::clamp(sy, 0.0F, maxSy);

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
			_surface->MarkDirty();
		}
	}
}
