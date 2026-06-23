#include "Application.h"
#include "Element.h"
#include "sandbox/RaylibWindow.h"
#include <iostream>

using namespace mocca;

std::function<void(int)> setter;

auto Counter(int start) -> Element
{
	auto [count, setCount] = useState(start);

	setter = setCount;

	useEffect([]() -> void { // do something
	});

	return text(std::format("Counter: {}", count));
}

auto buildExampleTree() -> Element
{
	using namespace mocca::styles;

	return box(
		BoxDescriptor{
			.Style = {.Width = {px(200)}, .Height = {px(500)}},
			.Children = {
				box(BoxDescriptor{
					.Style = {.Width = {percent(50)}, .Height = {px(50)}},
					.Children =
						{
							text("Hello"),
							component(Counter, 5),
						}
				}),
				box({
					.Style = {.Width = {px(50)}, .Height = {px(100)}},
					.Children =
						{
							text({.Key = "a", .Content = "Item A"}),
							text({.Key = "b", .Content = "Item B"}),
						},
				}),
				box({.Style = {.Width = {px(50)}, .Height = {px(50)}}}),
			},
		}
	);
}

void logCallback(const LogMessage& message, void* user)
{
	std::cout << message.Message << "\n";
}

auto main(int argc, const char** argv) -> int
{
	auto app = Application("com.mocca.sandbox");
	mocca::Logger::SetLogCallback(logCallback, 0);

	auto* window = app.RegisterSurface<RaylibWindow>(
		{.Width = 800,
		 .Height = 600,
		 .Title = "Sandbox",
		 .Root = buildExampleTree}
	);

	while (app.IsRunning())
	{
		app.Tick(0);
	}

	return 0;
}