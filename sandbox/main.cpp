#include "GLFWPlatformInterface.h"
#include "core/Application.h"
#include "core/Element.h"
#include "core/Hooks.h"
#include "gfx/Texture.h"
#include "sandbox/NVGRenderDevice.h"
#include <iostream>
#include "style/StyleHelpers.h"
#include "utils/Color.h"

using namespace mocca;

const char Placeholder[] = {
	"[latin wrap]      Lorem ipsum dolor sit amet, consectetur adipiscing "
	"elit, sed do eiusmod tempor incididunt ut labore et dolore magna "
	"aliqua.\n[kerning]         AVATAR To Ty Wa Yo\n[ligatures]       office "
	"affinity fluffy\n[tabular nums]    1111.11  8888.88  0123456789\n[long "
	"word]       "
	"Pneumonoultramicroscopicsilicovolcanoconiosis_and_a_very_long_snake_case_"
	"identifier_with_no_spaces\n[url]             "
	"https://example.com/a/very/long/path/with/no/break/"
	"opportunities?query=string&another=value\n[soft hyphen]     "
	"in­ter­na­tion­al­i­za­ti"
	"on          (U+00AD between syllables)\n[nbsp / zwsp]     100 km/h "
	"· a​b​c            (U+00A0 after 100, U+200B "
	"between a b c)\n[tabs]            a\tb\t\tc\n[arabic]          "
	"مرحبا "
	"بالعالم\n[arabic marks]  "
	"  "
	"السَّلَام"
	"ُ "
	"عَلَيْكُم"
	"ْ\n[lam-alef]        لا\n[hebrew]          "
	"שלום עולם\n[bidi "
	"mix]        The word سلام means \"peace\" and "
	"v2.0 עברית 123 mixed.\n[bidi numbers]  "
	"  الإصدار 2.0 "
	"من 100\n[emoji basic]     😀 🎉 "
	"❤️\n[emoji zwj]       "
	"👨‍👩‍👧"
	"‍👦 "
	"🏳️‍🌈\n[emoji "
	"modifier]  👍 👍🏽 "
	"👍🏿\n[flags]           "
	"🇯🇵 🇧🇷 "
	"🇩🇪\n[keycaps]         "
	"1️⃣ #️⃣ "
	"*️⃣\n[combining]       é (U+00E9)  vs  "
	"é (e + U+0301)  vs  "
	"Z͎͍a̷l̶g̵o̸\n[cjk]             "
	"你好，世界。"
	"日本語のテキ"
	"スト、折り返"
	"しのテスト。\n"
	"[chinese line]    "
	"鑑於對人類家庭所有成員的固有尊嚴及其平等的和不\n"
	"[chinese para]    "
	"人人生而自由，在尊嚴和權利上一律平等。"
	"他們賦有理性和良心，並應以兄弟關係的精神相對待。\n"
	"[hangul]          "
	"안녕하세요 "
	"세계\n[devanagari]      "
	"नमस्ते "
	"दुनिया\n"
	"[thai]            "
	"สวัสดี"
	"ชาวโลก\n"
	"[cyrillic/greek]  Привет, "
	"мир   Γειά "
	"σου "
	"Κόσμε\n[newlines]        line "
	"one\nline two\n\nline four after a blank "
	"line",
};

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
						BackgroundImage({"../sandbox/cat.png", "../sandbox/star.png"}),
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
				.Children = {
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
	}

	return 0;
}
