#include "Application.h"
#include "Detail.h"
#include "Element.h"

using namespace mocca;

auto buildExampleTree() -> Element
{
	return box({
		box({text("Hello")}),
		// counter goes here
		box({
			text("Item A", "a"),
			text("Item B", "b"),
		}),
		box({})
	});
}

auto buildExampleTree2() -> Element
{
	return box({box({text("world!")}),
				// counter goes here
				box({
					text("Item A", "a"),
					text("Item B", "b"),
					text("Item C", "c"),
				}),
				box({})});
}

auto main(int argc, const char** argv) -> int
{
	auto app = Application("com.mocca.sandbox");
	app.SetDefaultLogCallback();

	auto element = buildExampleTree();
	auto element2 = buildExampleTree2();
	auto tree = detail::Node::Reconcile(nullptr, &element);
	tree->Print();
	auto finalTree = detail::Node::Reconcile(std::move(tree), &element2);
	finalTree->Print();

	return 0;
}