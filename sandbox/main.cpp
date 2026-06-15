#include "Application.h"
#include "Element.h"
#include <iostream>

using namespace mocca;

std::function<void(int)> setter;

auto Counter(int start) -> Element
{
	auto [count, setCount] = useState(start);

	setter = setCount;

	useEffect([]() -> void { mc_info("hi"); });

	return text(std::format("Counter: {}", count));
}

auto buildExampleTree() -> Element
{
	return box({box({text("Hello")}), component(Counter, 5),
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

	app.Print();
	setter(1);
	app.Tick(0);
	app.Print();

	return 0;
}