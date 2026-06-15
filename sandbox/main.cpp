#include "Application.h"
#include "Element.h"
#include <iostream>

using namespace mocca;

std::function<void(int)> setter;

auto Counter() -> Element
{
	auto [count, setCount] = useState(0);

	setter = setCount;
	return text(std::format("Counter: {}", count));
}

auto buildExampleTree() -> Element
{
	return box({box({text("Hello")}), component(Counter),
				box({
					text("Item A", "a"),
					text("Item B", "b"),
				}),
				box({})});
}

void logCallback(const LogMessage& message, void* user)
{
	std::cout << message.Message << "\n";
}

auto main(int argc, const char** argv) -> int
{
	auto app = Application("com.mocca.sandbox");
	app.SetLogCallback(logCallback, 0);

	app.RegisterSurface<mocca::Surface>({.Root = buildExampleTree});

	setter(1);
	app.Tick(0);
	return 0;
}