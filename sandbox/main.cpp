#include "Application.h"
#include "Element.h"
#include "StyleHelpers.h"
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
			.Style =
				{
					.Width = {px(200)},
					.Height = {px(500)},
					.Margin = margin(Auto),
					.AlignContent = StyleKeyword::Inherit,
					.AlignItems = Alignment::Stretch,
					.AlignSelf = {Auto},
				},
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
					.Style =
						{.Width = {Auto},
						 .Height = {px(100)},
						 .Padding = padding(px(5)),
						 .Margin = margin(px(5))},
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

	auto* window = app.RegisterSurface<Surface>(
		{.Width = 800,
		 .Height = 600,
		 .Title = "Sandbox",
		 .Root = buildExampleTree}
	);

	app.Tick(0);

	mocca::Color color = mocca::Color::Oklch(0.55, 0.18, 240.0);
	mc_info("{:08x}", color.ToRgba());

	return 0;
}