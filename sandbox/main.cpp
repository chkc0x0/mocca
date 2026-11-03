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

struct Row
{
	int Id;
	std::string Label;
};

struct RowProps
{
	struct Row Row;
	std::function<void(mocca::PointerEvent&)> OnRemove;
};

std::vector<Row> brows;

auto RowComp(const RowProps& r) -> mocca::Element
{
	using namespace mocca;
	using namespace mocca::styles;

	return mocca::component(
		[](auto& r) -> Element
		{
			return box({
				.Style =
					{
						AlignSelf(Alignment::FlexStart),
						FlexShrink(0),
						BackgroundColor(mocca::colors::White),
						FlexDirection(FlexDirections::Row),
						JustifyContent(Justification::SpaceBetween),
						Width(percent(100)),
						MinHeight(FitContent),
						Padding(px(10)),
						BorderRadius(px(10)),
						AlignItems(Alignment::Center),
					},
				.Children = {
					box({
						.Style =
							{
								GapHorizontal(px(10)),
							},
						.Children =
							{
								box({
									.Style =
										{
											Width(px(24)),
											Height(px(24)),
											BackgroundImage(
												"../sandbox/cat.png"
											),
										},
								}),
								text(r.Row.Label),
							},
					}),
					widgets::basic::button({
						.State = widgets::ButtonVariant::Destructive,
						.Children = {text("✗")},
						.OnPress = r.OnRemove,
					}),
				},
			});
		},
		r,
		{
			.Key = r.Row.Id,
		}
	);
}

auto Bench() -> mocca::Element
{
	using namespace mocca;
	using namespace mocca::styles;

	auto [rows, setRows] = useState<std::vector<Row>>(brows);
	auto [counter, setCounter] = useState(0);

	return mocca::box({
		.Style =
			{
				TextColor(mocca::colors::AlmostBlack),
				Padding(px(16)),
				FontFamily("Inter"),
			},
		.Children = {
			box({
				.Events =
					{
						on(events::PointerDown,
						   [setCounter](auto& ev) -> auto
						   {
							   setCounter(
								   [](auto& c) -> auto { return c + 1; }
							   );
						   }),
					},
				.Children =
					{
						text("Counter: " + std::to_string(counter)),
					},
			}),
			box({
				.Style =
					{
						FlexDirection(FlexDirections::Column),
						GapVertical(px(10)),
						Width(percent(100)),
						Overflow(OverflowType::Scroll),
					},
				.Children = mapElements(
					rows,
					[setRows](auto& r) -> auto
					{
						return RowComp({
							.Row = r,
							.OnRemove = [setRows, r](auto& ev) -> auto
							{
								setRows(
									[id = r.Id](std::vector<Row> cur) -> auto
									{
										std::erase_if(
											cur,
											[&](const Row& r) -> auto
											{ return r.Id == id; }
										);
										return cur;
									}
								);
							},
						});
					}
				),
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

	for (int i = 0; i < 8000; i++)
	{
		brows.push_back({.Id = i, .Label = "Label " + std::to_string(i + 1)});
	}

	app.RegisterSurface({
		.Width = 800,
		.Height = 600,
		.Title = "Sandbox",
		.Flags = SurfaceFlags::External,
		.Root = Bench,
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
