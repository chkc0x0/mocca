#include "widgets/Button.h"
#include "anim/Curve.h"
#include "core/Hooks.h"
#include "style/StyleBuilder.h"
#include "style/StyleHelpers.h"
#include "style/StyleList.h"
#include "utils/Color.h"
#include "widgets/ComponentTypes.h"
#include <utility>

namespace mocca::widgets
{
	auto buttonImpl(const ButtonProps& props) -> Element
	{
		auto* node = useNode();

		auto press = usePress(
			{.Ref = node, .Disabled = props.Disabled, .OnPress = props.OnPress}
		);

		styles::StyleList base = {
			styles::AlignSelf(styles::Alignment::FlexStart),
			styles::CursorShape(
				props.Disabled ? CursorShapeType::NotAllowed
							   : CursorShapeType::Pointer
			)
		};

		{
			Color bg;
			Color fg = Color(250, 250, 250);

			if (!props.Disabled)
			{
				switch (props.Variant)
				{
				case ButtonVariant::Default:
					bg = Color(237, 237, 237);
					fg = Color(23, 23, 23);
					break;

				case ButtonVariant::Secondary:
					bg = Color(38, 38, 38);
					break;

				case ButtonVariant::Outline:
					bg = Color(35, 35, 35);
					break;

				case ButtonVariant::Ghost:
					bg = Color(32, 32, 32, 0);
					break;

				case ButtonVariant::Destructive:
					bg = Color(165, 61, 64);
					break;
				}
			}
			else
			{
				fg = Color(138, 138, 138);
				switch (props.Variant)
				{
				case ButtonVariant::Default:
					bg = Color(127, 127, 127);
					fg = Color(24, 24, 24);
					break;

				case ButtonVariant::Secondary:
					bg = Color(32, 32, 32);
					break;

				case ButtonVariant::Outline:
					bg = Color(30, 30, 30);
					break;

				case ButtonVariant::Ghost:
					bg = colors::Transparent;
					break;

				case ButtonVariant::Destructive:
					bg = Color(95, 43, 45);
					break;
				}
			}

			base += {
				styles::Padding(styles::px(8), styles::px(16)),
				styles::TextColor(fg),
				styles::BackgroundColor(bg),
				styles::BorderRadius(styles::px(10)),
				styles::Transition(0.45F, anim::presets::cubic::OutQuint),
				styles::Border(styles::px(2)),
				styles::Transition(
					styles::StyleProperty::Scale,
					anim::presets::springs::Lively
				)
			};

			base.AddIf(
				press.State.Hovered,
				{styles::BackgroundColor(
					props.Variant == ButtonVariant::Default ? bg.Darkened(0.1)
															: bg.Lightened(0.05)
				)}
			);

			base.AddIf(
				props.Variant == ButtonVariant::Ghost && press.State.Hovered,
				{styles::BackgroundColor(Color(32, 32, 32))}
			);

			base.AddIf(
				props.Variant == ButtonVariant::Outline,
				{
					styles::BorderColor(Color(67, 67, 67)),
				}
			);

			base.AddIf(
				props.Variant == ButtonVariant::Outline && props.Disabled,
				{
					styles::BorderColor(Color(43, 43, 43)),
				}
			);

			base.AddIf(
				props.Variant == ButtonVariant::Outline && props.Disabled,
				{
					styles::BorderColor(Color(43, 43, 43)),
				}
			);

			base.AddIf(
				node->IsFocusVisible(),
				{
					styles::BorderColor(Color(0, 0, 255)),
				}
			);

			base.AddIf(press.State.Pressed, {styles::Scale(0.95F)});
		}

		return box({
			.A11y = props.A11y != nullptr ? props.A11y
										  : a11y({
												.Role = aria::Role::Button,
												.Disabled = props.Disabled,
											}),
			.Style = base,
			.Events = std::move(press.Events),
			.Children = props.Children,
			.Ref = node,
			.Focus = props.Disabled ? FocusMode::None : FocusMode::Tab,
		});
	}

	auto button(ButtonProps props) -> Element
	{
		return component(
			buttonImpl,
			std::move(props),
			{.Key = props.Key, .Style = props.Style}
		);
	}

	auto button(const std::string& text, std::function<void(PointerEvent&)> cb)
		-> Element
	{
		return button({
			.Children = {mocca::text(text)},
			.OnPress = std::move(cb),
		});
	}
}