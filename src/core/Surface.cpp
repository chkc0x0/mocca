#include "Surface.h"
#include "Logger.h"
#include "Style.h"
#include "yoga/YGNode.h"

namespace mocca
{
	void Surface::Tick(double dt)
	{
		auto tree = Element::Render(_desc.Root, 0);
		_root = detail::Node::Reconcile(std::move(_root), &tree);

		if (!_root)
		{
			return;
		}

		_root->ComputeStyle(styles::DefaultStyle);

		_root->BuildYogaTree();
		YGNodeCalculateLayout(
			_root->YogaNode,
			_desc.Width,
			_desc.Height,
			YGDirectionLTR
		);
	}

	void Surface::Paint()
	{
		if (!_root || !_dirty)
		{
			return;
		}

		_canvas.Clear();
		_root->Paint(_canvas);

		_dirty = false;
	}

	void Surface::Print(int depth)
	{
		std::string indent(static_cast<size_t>(depth * 2), ' ');

		mc_info("{}[Surface]", indent);
		_root->Print(depth + 1);
	}
}