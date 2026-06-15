#include "Application.h"
#include "Detail.h"
#include "Element.h"

using namespace mocca;

std::function<void(int)> setter;

auto buildExampleTree() -> Element
{
	auto [count, setCount] = useState(0);

	setter = setCount;

	return box({
		box({text("Hello")}),
		text(std::format("Count: {}", count)),
		box({
			text("Item A", "a"),
			text("Item B", "b"),
		}),
		box({})
	});
}

auto main(int argc, const char** argv) -> int
{
	auto app = Application("com.mocca.sandbox");
	app.SetDefaultLogCallback();

	auto element = buildExampleTree();
	auto tree = detail::Node::Reconcile(nullptr, &element);
	tree->Print();
	setter(1);
	app.Tick(0);
	tree->Print();

	return 0;
}