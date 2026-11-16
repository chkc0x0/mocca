#include "widgets/Switch.h"
#include "core/Hooks.h"
#include "style/StyleBuilder.h"
#include "widgets/ComponentTypes.h"
#include <utility>

namespace mocca::widgets
{
	auto switchImpl(const SwitchProps& props) -> Element
	{
		auto [value, setValue] = useState(props.DefaultValue);

		auto* node = useNode();

		auto press = usePress({
			.Ref = node,
			.Disabled = props.Disabled,
			.OnPress = [setValue, props](auto& ev) -> auto
			{
				bool newVal;
				setValue(
					[&newVal](bool value) -> auto
					{
						newVal = !value;
						return !value;
					}
				);

				if (props.OnValueChange)
				{
					props.OnValueChange(newVal);
				}
			},
		});

		return box({
			.Style =
				{
					styles::GapHorizontal(10),
					styles::Width(styles::FitContent),
				},
			.Events = std::move(press.Events),
			.Children = children(
				box({
					.Style =
						{
							styles::Width(33),
							styles::Height(18),
							styles::BorderRadius(styles::percent(50)),
							styles::BackgroundColor(
								Color(value ? 0xe5e5e5 : 0x353535)
							),
							styles::CursorShape(
								props.Disabled ? CursorShapeType::NotAllowed
											   : CursorShapeType::Pointer
							),
							styles::Transition(
								styles::StyleProperty::All,
								anim::presets::springs::Smooth
							),
						},
					.Parts =
						{
							part(
								"thumb",
								{
									styles::Width(
										press.State.Hovered ? 18 : 14
									),
									styles::Height(14),
									styles::Left(value ? 17 : 2),
									styles::Right(2),
									styles::Top(2),
									styles::BackgroundColor(
										Color(value ? 0x171717 : 0xfafafa)
									),
									styles::BorderRadius(styles::percent(50)),
									styles::Transition(
										styles::StyleProperty::All,
										anim::presets::springs::Lively
									),
								}
							),
						},
				}),
				props.Children
			),
			.Ref = node,
		});
	}

	auto Switch(SwitchProps props) -> Element
	{
		return component(switchImpl, std::move(props), {.Key = props.Key});
	}
}