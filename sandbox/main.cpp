#include "SDL3PlatformInterface.h"
#include "core/Application.h"
#include "core/Element.h"
#include "sandbox/NVGRenderDevice.h"
#include <iostream>
#include "style/StyleBuilder.h"
#include "style/StyleHelpers.h"
#include "utils/Color.h"
#include "widgets/Button.h"
#include "widgets/Switch.h"

using namespace mocca;

auto buttonShowcase() -> Element
{
	using namespace mocca::styles;

	std::vector<widgets::ButtonProps> buttons = {
		{.Variant = widgets::ButtonVariant::Default},
		{.Variant = widgets::ButtonVariant::Secondary},
		{.Variant = widgets::ButtonVariant::Outline},
		{.Variant = widgets::ButtonVariant::Ghost},
		{.Variant = widgets::ButtonVariant::Destructive},
	};

	auto press = [](auto& ev) -> auto {};

	std::vector<Element> childrenNormal;
	std::vector<Element> disabledChildren;

	childrenNormal.reserve(buttons.size());

	for (auto& btn : buttons)
	{
		childrenNormal.push_back(
			widgets::button({
				.Children =
					{
						text("Click me!"),
					},
				.Variant = btn.Variant,
				.OnPress = press,
			})
		);
	}

	disabledChildren.reserve(buttons.size());
	for (auto& btn : buttons)
	{
		disabledChildren.push_back(
			widgets::button({
				.Children =
					{
						text("Click me!"),
					},
				.Variant = btn.Variant,
				.Disabled = true,
				.OnPress = press,
			})
		);
	}

	return box({
		.Style =
			{
				GapVertical(px(16)),
				FlexDirection(FlexDirections::Column),
			},
		.Children = {
			box({
				.Style =
					{
						FlexDirection(FlexDirections::Row),
						GapHorizontal(px(16)),
					},
				.Children = childrenNormal,
			}),
			box({
				.Style =
					{
						FlexDirection(FlexDirections::Row),
						GapHorizontal(px(16)),
					},
				.Children = disabledChildren,
			}),
		},
	});
}

auto Sandbox() -> Element
{
	using namespace mocca::styles;

	return box({
		.Style =
			{
				BackgroundColor(Color(0x191919)),
				FontFamily("Inter"),
				Padding(px(16)),
				GapVertical(px(16)),
				FlexDirection(FlexDirections::Column),
			},
		.Children = {
			buttonShowcase(),
			widgets::Switch({
				.Children = {
					text("Airplane mode"),
				},
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
	app.SetPlatformInterface(sandbox::createSDL3PlatformInterface());

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
