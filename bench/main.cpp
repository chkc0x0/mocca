#include "core/Application.h"
#include "core/Hooks.h"
#include "input/InputTypes.h"
#include "style/StyleEnums.h"
#include "style/StyleHelpers.h"
#include "utils/Color.h"
#include "utils/ScopedProfiler.h"
#include "widgets/Button.h"

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
	mc_profileFunction();
	using namespace mocca;
	using namespace mocca::styles;

	auto [rows, setRows] = useState<std::vector<Row>>(brows);

	return mocca::box({
		.Style =
			{
				TextColor(mocca::colors::AlmostBlack),
				Padding(px(16)),
				FontFamily("Inter"),
			},
		.Children = {
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

auto main(int argc, const char** argv) -> int
{
	mocca::Application app{"com.mocca.bench"};

	app.Fonts().RegisterFile("../sandbox/Inter.ttf");

	app.RegisterSurface({.Width = 800, .Height = 800, .Root = Bench});
	for (int i = 0; i < 1000; i++)
	{
		brows.push_back({.Id = i, .Label = "Label " + std::to_string(i + 1)});
	}

	app.Tick();
	// app.Print();
	return 0;
}