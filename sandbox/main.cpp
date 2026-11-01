#include "GLFWPlatformInterface.h"
#include "core/Application.h"
#include "core/Element.h"
#include "sandbox/NVGRenderDevice.h"
#include <iostream>
#include "style/StyleHelpers.h"
#include "utils/Color.h"
#include "widgets/Button.h"

using namespace mocca;

auto Sandbox() -> Element
{
	using namespace mocca::styles;

	return box({
		.Style =
			{
				BackgroundColor(Color::Oklch(0.7, 0.1036, 171)),
				FontFamily("Inter"),
			},
		.Children = {
			widgets::basic::button({
				.Children = {
					text("Click me!"),
				},
				.OnPress = [](auto& ev) { mc_info("clicked"); },
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
