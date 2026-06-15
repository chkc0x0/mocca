#pragma once
#include "Detail.h"
#include "Element.h"
#include <memory>

namespace mocca
{
	struct SurfaceDesc
	{
		ComponentFn Root;
	};

	class Surface
	{
	public:
		Surface(const SurfaceDesc& desc)
		{
			auto tree = Element::Render(desc.Root, 0);
			_root = detail::Node::Reconcile(std::move(_root), &tree);
			_rootFn = desc.Root;
		}

		void Tick(double dt);
		void Print(int depth = 0);

	private:
		ComponentFn _rootFn;
		std::unique_ptr<detail::Node> _root = nullptr;
	};
}