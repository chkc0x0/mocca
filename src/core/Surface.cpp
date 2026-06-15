#include "Surface.h"
#include "Application.h"
#include "Logger.h"

namespace mocca
{
	void Surface::Tick(double dt)
	{
		auto tree = Element::Render(_rootFn, 0);
		_root = detail::Node::Reconcile(std::move(_root), &tree);
	}
	
	void Surface::Print(int depth)
	{
		std::string indent(static_cast<size_t>(depth * 2), ' ');

		mc_info("{}[Surface]", indent);
		_root->Print(depth + 1);
	}
}