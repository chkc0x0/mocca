#include "Application.h"
#include "Element.h"
#include "StyleHelpers.h"
#include "sandbox/GLFWPlatformSurface.h"
#include <iostream>
#include "GLFW/glfw3.h"

using namespace mocca;

auto Counter(int start) -> Element
{
	using namespace mocca::styles;

	auto [count, setCount] = useState(start);

	return box(
		BoxDescriptor{
			.Style =
				{
					.Width = {percent(50)},
					.Height = {percent(100)},
					.AlignItems = Alignment::Center,
					.JustifyContent = Justification::Center,
				},
			.Events = {
				.OnPointerDown = [setCount](auto& ev) -> auto
				{ setCount([](auto prev) -> auto { return prev + 1; }); },
				.OnKeyDown = [](auto& ev) -> auto
				{ mc_info("key {}", (int)ev.Code); },
				.OnTextInput = [](auto& ev) -> auto
				{
					mc_info(
						"codepoint {} {}",
						(uint32_t)ev.Codepoint,
						(char)ev.Codepoint
					);
				},
			},
			.Children = {
				text({
					.Content = std::format("Counter: {}", count),
				}),
			},
		}
	);
}

auto TextField() -> Element
{
	using namespace mocca::styles;
	auto [textContent, setText] = useState(std::u32string{});

	return box(
		BoxDescriptor{
			.Style =
				{
					.Width = {percent(50)},
					.Height = {percent(100)},
					.Padding = padding(px(4)),
					.AlignItems = Alignment::FlexStart,
					.JustifyContent = Justification::Center,
				},
			.Events = {
				.OnKeyDown = [setText](auto& ev) -> auto
				{
					if (ev.Code == KeyCode::Backspace)
					{
						setText(
							[](std::u32string prev) -> auto
							{
								if (!prev.empty())
								{
									prev.pop_back();
								}
								return prev;
							}
						);
					}
				},
				.OnTextInput = [setText](auto& ev) -> auto
				{
					setText(
						[ev](std::u32string prev) -> auto
						{
							prev.push_back(ev.Codepoint);
							return prev;
						}
					);
				},
			},
			.Children = {
				text({.Content = mocca::detail::toUtf8(textContent)}),
			},
		}
	);
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
					.AlignContent = StyleKeyword::Inherit,
					.AlignItems = Alignment::Stretch,
					.AlignSelf = {Auto},
				},
			.Children = {
				box(BoxDescriptor{
					.Style =
						{
							.Width = {percent(100)},
							.Height = {px(50)},
							.FlexDirection = FlexDirections::Row,
						},
					.Children =
						{
							component(Counter, 1),
							component(TextField),
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
