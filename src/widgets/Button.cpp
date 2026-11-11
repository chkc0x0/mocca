#include "widgets/Button.h"
#include "core/Hooks.h"
#include "style/StyleBuilder.h"
#include "style/StyleHelpers.h"
#include "style/StyleList.h"
#include "utils/Color.h"

#include <utility>

namespace mocca::widgets::basic
{
	auto usePress(const PressOptions& opts) -> PressResult
	{
		auto [state, setState] = useState(PressState{});
		auto armed = useRef(false);

		auto disabled = opts.Disabled;
		auto onPress = opts.OnPress;

		return {
			.State = state,
			.Events = {
				on(events::PointerEnter,
				   [=](PointerEvent&) -> auto
				   {
					   if (!disabled && !state.Hovered)
					   {
						   setState(
							   {.Hovered = true, .Pressed = state.Pressed}
						   );
					   }
				   }),
				on(events::PointerLeave,
				   [armed, setState](PointerEvent&) -> auto
				   {
					   armed.Current = false;
					   setState(PressState{});
				   }),
				on(events::PointerDown,
				   [armed, setState, disabled](PointerEvent&) -> auto
				   {
					   if (disabled)
					   {
						   return;
					   }
					   armed.Current = true;
					   setState({.Hovered = true, .Pressed = true});
				   }),
				on(events::PointerUp,
				   [armed, setState, disabled, onPress, state](PointerEvent& ev)
					   -> auto
				   {
					   bool fire = armed.Current && !disabled;
					   armed.Current = false;
					   setState({.Hovered = state.Hovered, .Pressed = false});
					   if (fire && onPress)
					   {
						   onPress(ev);
					   }
				   }),
			},
		};
	}

	auto buttonImpl(const ButtonProps& props) -> Element
	{
		auto press = usePress(
			{.Disabled = props.Disabled, .OnPress = props.OnPress}
		);

		auto* node = useNode();

		auto disabled = props.Disabled;
		auto onPress = props.OnPress;

		EventView events = std::move(press.Events);
		events.Add(
			on(events::Activate,
			   [disabled, onPress, node](ActivateEvent&) -> void
			   {
				   if (disabled || !onPress)
				   {
					   return;
				   }

				   auto r = node->Rect();
				   PointerEvent synth{};
				   synth.EventType = PointerEvent::Type::Up;
				   synth.X = r.X + (r.Width * 0.5F);
				   synth.Y = r.Y + (r.Height * 0.5F);
				   onPress(synth);
			   })
		);

		styles::StyleList base = {
			styles::AlignSelf(styles::Alignment::FlexStart),
			styles::CursorShape(
				props.Disabled ? CursorShapeType::NotAllowed
							   : CursorShapeType::Pointer
			)
		};

		// styles - remove these or simplify them later, as they go in eclipse widgets
		{
			Color bg;
			Color fg = Color(250, 250, 250);

			if (!props.Disabled)
			{
				switch (props.State)
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
				switch (props.State)
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
				styles::Transition(0.45F, anim::bezier(0.23F, 1, 0.32F, 1)),
				styles::Border(styles::px(2)),
			};

			base.AddIf(
				press.State.Hovered,
				{styles::BackgroundColor(
					props.State == ButtonVariant::Default ? bg.Darkened(0.1)
														  : bg.Lightened(0.05)
				)}
			);

			base.AddIf(
				props.State == ButtonVariant::Ghost && press.State.Hovered,
				{styles::BackgroundColor(Color(32, 32, 32))}
			);

			base.AddIf(
				props.State == ButtonVariant::Outline,
				{
					styles::BorderColor(Color(67, 67, 67)),
				}
			);

			base.AddIf(
				props.State == ButtonVariant::Outline && props.Disabled,
				{
					styles::BorderColor(Color(43, 43, 43)),
				}
			);

			base.AddIf(
				props.State == ButtonVariant::Outline && props.Disabled,
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
		}

		return box({
			.A11y = props.A11y != nullptr ? props.A11y
										  : a11y({
												.Role = aria::Role::Button,
												.Disabled = props.Disabled,
											}),
			.Style = base + props.Style,
			.Events = std::move(events),
			.Children = props.Children,
			.Ref = node,
			.Focus = props.Disabled ? FocusMode::None : FocusMode::Tab,
		});
	}

	auto button(ButtonProps props) -> Element
	{
		return component(buttonImpl, std::move(props));
	}
}