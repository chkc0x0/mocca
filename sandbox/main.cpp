#include "GLFWPlatformInterface.h"
#include "core/Application.h"
#include "core/Element.h"
#include "core/Hooks.h"
#include "gfx/Texture.h"
#include "sandbox/NVGRenderDevice.h"
#include <fstream>
#include <iostream>
#include "style/StyleHelpers.h"
#include "utils/Color.h"

using namespace mocca;
std::string Placeholder;

auto Editor() -> Element
{
	using namespace mocca::styles;

	auto [draft, setDraft] = useState(std::string(Placeholder));

	auto edit = [setDraft](const KeyEvent& keys) -> void
	{
		if (keys.Code == KeyCode::Backspace)
		{
			setDraft(
				[](std::string prev) -> auto
				{
					if (!prev.empty())
					{
						prev.pop_back();
					}
					return prev;
				}
			);
		}
		else if (keys.Code == KeyCode::Enter)
		{
			setDraft(
				[](std::string prev) -> auto
				{
					prev.push_back('\n');
					return prev;
				}
			);
		}
	};

	return box({
		.Style =
			{
				BackgroundColor(Color::Oklch(0.7, 0.1036, 171)),
				Padding(px(16)),
				FontFamily(
					"Inter",
					"Noto Emoji",
					"Google Sans",
					"Noto Sans Arabic",
					"Noto Sans TC"
				),
				FontSize(px(24)),
			},
		.Children = {
			box({
				.Style =
					{
						BackgroundImage(
							{"../sandbox/cat.png", "../sandbox/star.png"}
						),
						BackgroundColor(Color(255, 255, 255, 40)),
						BorderRadius(px(16)),
						Width(px(600)),
						Height(px(500)),
						Padding(px(16), px(0), px(16)),
						AlignItems(Alignment::Stretch),
						JustifyContent(Justification::FlexStart),
					},
				.Events = {
					.OnKeyDown = [edit](auto& ev) -> auto { edit(ev); },
					.OnKeyRepeat = [edit](auto& ev) -> auto { edit(ev); },
					.OnTextInput = [setDraft](auto& ev) -> auto
					{
						setDraft(
							[ev](std::string prev) -> auto
							{
								prev += ev.Text;
								return prev;
							}
						);
					},
				},
				.Children =
					{
						text(
							TextDescriptor{
								.Style =
									{
										TextColor(Color(255, 255, 255)),
										FontWeight(500.0F),
										LineHeight(px(26)),
										LetterSpacing(px(0.25F)),
										TextAlign(TextAlignment::Start),
									},
								.Content = draft,
							}
						),
					},
				.Focus = FocusMode::Tab,
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

	app.Fonts().RegisterFile(
		"../sandbox/GoogleSans.ttf",
		{.Family = "Google Sans"}
	);

	app.Fonts().RegisterFile(
		"../sandbox/NotoEmoji.ttf",
		{.Family = "Noto Emoji"}
	);

	app.Fonts().RegisterFile(
		"../sandbox/NotoSansArabic.ttf",
		{.Family = "Noto Sans Arabic"}
	);

	app.Fonts().RegisterFile(
		"../sandbox/NotoSansTC.ttf",
		{.Family = "Noto Sans TC"}
	);

	std::ifstream file("../sandbox/text.txt");
	std::stringstream ss;
	ss << file.rdbuf();
	Placeholder = ss.str();
	file.close();

	app.RegisterSurface({
		.Width = 800,
		.Height = 600,
		.Title = "Sandbox",
		.Flags = SurfaceFlags::External,
		.Root = Editor,
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
