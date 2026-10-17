#include "core/Application.h"
#include "core/Element.h"
#include "core/Hooks.h"
#include "sandbox/GLFWPlatformSurface.h"
#include <iostream>
#include "GLFW/glfw3.h"
#include "style/StyleHelpers.h"
#include "utils/Color.h"

using namespace mocca;

const char placeholder[] = {
	"[latin wrap]      Lorem ipsum dolor sit amet, consectetur adipiscing "
	"elit, sed do eiusmod tempor incididunt ut labore et dolore magna "
	"aliqua.\n[kerning]         AVATAR To Ty Wa Yo\n[ligatures]       office "
	"affinity fluffy\n[tabular nums]    1111.11  8888.88  0123456789\n[long "
	"word]       "
	"Pneumonoultramicroscopicsilicovolcanoconiosis_and_a_very_long_snake_case_"
	"identifier_with_no_spaces\n[url]             "
	"https://example.com/a/very/long/path/with/no/break/"
	"opportunities?query=string&another=value\n[soft hyphen]     "
	"in\302\255ter\302\255na\302\255tion\302\255al\302\255i\302\255za\302\255ti"
	"on          (U+00AD between syllables)\n[nbsp / zwsp]     100 km/h "
	"\302\267 a\342\200\213b\342\200\213c            (U+00A0 after 100, U+200B "
	"between a b c)\n[tabs]            a\tb\t\tc\n[arabic]          "
	"\331\205\330\261\330\255\330\250\330\247 "
	"\330\250\330\247\331\204\330\271\330\247\331\204\331\205\n[arabic marks]  "
	"  "
	"\330\247\331\204\330\263\331\216\331\221\331\204\331\216\330\247\331\205"
	"\331\217 "
	"\330\271\331\216\331\204\331\216\331\212\331\222\331\203\331\217\331\205"
	"\331\222\n[lam-alef]        \331\204\330\247\n[hebrew]          "
	"\327\251\327\234\327\225\327\235 \327\242\327\225\327\234\327\235\n[bidi "
	"mix]        The word \330\263\331\204\330\247\331\205 means \"peace\" and "
	"v2.0 \327\242\327\221\327\250\327\231\327\252 123 mixed.\n[bidi numbers]  "
	"  \330\247\331\204\330\245\330\265\330\257\330\247\330\261 2.0 "
	"\331\205\331\206 100\n[emoji basic]     \360\237\230\200 \360\237\216\211 "
	"\342\235\244\357\270\217\n[emoji zwj]       "
	"\360\237\221\250\342\200\215\360\237\221\251\342\200\215\360\237\221\247"
	"\342\200\215\360\237\221\246 "
	"\360\237\217\263\357\270\217\342\200\215\360\237\214\210\n[emoji "
	"modifier]  \360\237\221\215 \360\237\221\215\360\237\217\275 "
	"\360\237\221\215\360\237\217\277\n[flags]           "
	"\360\237\207\257\360\237\207\265 \360\237\207\247\360\237\207\267 "
	"\360\237\207\251\360\237\207\252\n[keycaps]         "
	"1\357\270\217\342\203\243 #\357\270\217\342\203\243 "
	"*\357\270\217\342\203\243\n[combining]       \303\251 (U+00E9)  vs  "
	"\303\251 (e + U+0301)  vs  "
	"Z\315\216\315\215a\314\267l\314\266g\314\265o\314\270\n[cjk]             "
	"\344\275\240\345\245\275\357\274\214\344\270\226\347\225\214\343\200\202"
	"\346\227\245\346\234\254\350\252\236\343\201\256\343\203\206\343\202\255"
	"\343\202\271\343\203\210\343\200\201\346\212\230\343\202\212\350\277\224"
	"\343\201\227\343\201\256\343\203\206\343\202\271\343\203\210\343\200\202\n"
	"[hangul]          "
	"\354\225\210\353\205\225\355\225\230\354\204\270\354\232\224 "
	"\354\204\270\352\263\204\n[devanagari]      "
	"\340\244\250\340\244\256\340\244\270\340\245\215\340\244\244\340\245\207 "
	"\340\244\246\340\245\201\340\244\250\340\244\277\340\244\257\340\244\276\n"
	"[thai]            "
	"\340\270\252\340\270\247\340\270\261\340\270\252\340\270\224\340\270\265"
	"\340\270\212\340\270\262\340\270\247\340\271\202\340\270\245\340\270\201\n"
	"[cyrillic/greek]  \320\237\321\200\320\270\320\262\320\265\321\202, "
	"\320\274\320\270\321\200   \316\223\316\265\316\271\316\254 "
	"\317\203\316\277\317\205 "
	"\316\232\317\214\317\203\316\274\316\265\n[newlines]        line "
	"one\342\217\216line two\342\217\216\342\217\216line four after a blank "
	"line",
};

auto Editor() -> Element
{
	using namespace mocca::styles;

	auto [draft, setDraft] = useState(
		std::string(placeholder)
	);

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
			},
		.Children = {
			box({
				.Style =
					{
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
									FontSize(px(18)),
									FontWeight(500.0F),
									LineHeight(px(26)),
									LetterSpacing(px(0.25F)),
									TextAlign(TextAlignment::Start),
								},
							.Content = draft,
						}
					),
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
	mocca::Logger::SetLogCallback(logCallback);

	app.Fonts().RegisterFile(
		"../sandbox/Inter.ttf",
		{
			.Family = "Inter",
			.Weight = Range<float>{100, 900},
		}
	);

	app.On(
		ApplicationEvent::SurfaceCreated,
		[](auto* data, auto* user) -> auto
		{
			auto* surface = (Surface*)data;
			if (surface->IsExternal())
			{
				surface->SetPlatform<GLFWPlatformSurface>();
			}
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
