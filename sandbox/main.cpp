#include "Application.h"
#include "Element.h"
#include "StyleHelpers.h"
#include "sandbox/GLFWPlatformSurface.h"
#include <iostream>
#include "GLFW/glfw3.h"

using namespace mocca;

auto Counter(int start) -> Element
{
	auto [count, setCount] = useState(start);

	return text({
		.Content = std::format(
			"Counter: {}, Kidding, it's {}",
			count + 1,
			start
		),
	});
}

auto buildExampleTree() -> Element
{
	auto [count, setCount] = useState(0);

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
					.Events =
						{
							.OnPointerDown = [setCount](auto& ev) -> auto
							{
								setCount(
									[](auto prev) -> auto { return prev + 1; }
								);
							},
						},
					.Children =
						{
							text("Hello"),
							component(Counter, count),
						}
				}),
				box({
					.Style =
						{
							.Width = {Auto},
							.Height = {px(100)},
							.Padding = padding(px(5)),
							.Margin = margin(px(5)),
							.AlignItems = Alignment::FlexStart,
						},
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
	mocca::Logger::SetLogCallback(logCallback);

	app.On(
		ApplicationEvent::SurfaceCreated,
		[](auto* data, auto* user) -> auto
		{
			auto* surface = (Surface*)data;
			surface->SetPlatform<GLFWPlatformSurface>();
			return true;
		}
	);

	app.On(
		ApplicationEvent::Poll,
		[](auto* data, auto* user) -> auto
		{
			glfwPollEvents();
			return true;
		}
	);

	app.RegisterSurface({
		.Width = 800,
		.Height = 600,
		.Title = "Sandbox",
		.Root = buildExampleTree,
	});

	app.RegisterSurface({
		.Width = 800,
		.Height = 600,
		.Title = "Sandbox2",
		.Root = buildExampleTree,
	});

	app.Tick(0);

	app.Print();

	while (app.IsRunning())
	{
		app.Tick(0);
	}

	return 0;
}
