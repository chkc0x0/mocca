#include "GLFWPlatformInterface.h"
#include "core/Application.h"
#include "core/Element.h"
#include "core/Hooks.h"
#include "sandbox/NVGRenderDevice.h"
#include <iostream>
#include "style/StyleHelpers.h"
#include "utils/Color.h"
#include "widgets/Button.h"
#include "widgets/ComponentTypes.h"

using namespace mocca;

auto Sandbox() -> Element
{
	using namespace mocca::styles;

	auto [counter, setCounter] = useState(0);

	std::vector<widgets::basic::ButtonProps> buttons = {
		{.State = widgets::ButtonVariant::Default},
		{.State = widgets::ButtonVariant::Secondary},
		{.State = widgets::ButtonVariant::Outline},
		{.State = widgets::ButtonVariant::Ghost},
		{.State = widgets::ButtonVariant::Destructive},
	};

	auto press = [](auto& ev) -> auto { mc_info("clicked"); };

	std::vector<Element> childrenNormal;
	std::vector<Element> disabledChildren;

	childrenNormal.reserve(buttons.size());

	for (auto& btn : buttons)
	{
		childrenNormal.push_back(
			widgets::basic::button({
				.State = btn.State,
				.Children =
					{
						text("Click me!"),
					},
				.OnPress = press,
			})
		);
	}

	disabledChildren.reserve(buttons.size());
	for (auto& btn : buttons)
	{
		disabledChildren.push_back(
			widgets::basic::button({
				.State = btn.State,
				.Disabled = true,
				.Children =
					{
						text("Click me!"),
					},
				.OnPress = press,
			})
		);
	}

	return box({
		.Style =
			{
				BackgroundColor(Color(0x191919)),
				FontFamily("Inter"),
				Padding(px(16)),
				GapHorizontal(px(16)),
			},
		.Children = {
			box({
				.Style =
					{

						FlexDirection(FlexDirections::Column),
						GapVertical(px(16)),
					},
				.Children = childrenNormal,
			}),
			box({
				.Style =
					{
						FlexDirection(FlexDirections::Column),
						GapVertical(px(16)),
					},
				.Children = disabledChildren,
			}),
			widgets::basic::button({
				.Children =
					{
						text("Counter: " + std::to_string(counter)),
					},
				.OnPress = [setCounter](auto& ev) -> auto
				{ setCounter([](auto& c) -> auto { return c + 1; }); },
			}),
		},
	});
}

void logCallback(const LogMessage& message, void* user)
{
	std::cout << message.Message << "\n";
}

auto main(int argc, const char** argv) -> int
{
	auto app = Application("com.mocca.sandbox");
	app.SetRenderDevice<sandbox::NVGRenderDevice>();
	app.SetPlatformInterface(sandbox::createGLFWPlatformInterface());

	mocca::Logger::SetLogCallback(logCallback);

	app.Fonts().RegisterFile(
		"../sandbox/Inter.ttf",
		{
			.Family = "Inter",
			.Weight = Range<float>{100, 900},
		}
	);

	app.RegisterSurface({
		.Width = 800,
		.Height = 600,
		.Title = "Sandbox",
		.Flags = SurfaceFlags::External,
		.Root = Sandbox,
	});

	app.Tick(0);
	app.Print();

	while (app.IsRunning())
	{
		app.Tick();
		app.Render();
	}

	return 0;
}
