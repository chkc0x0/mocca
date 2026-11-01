#include "widgets/Button.h"
#include "core/Hooks.h"
#include "style/StyleBuilder.h"
#include "style/StyleList.h"

#include <utility>

namespace mocca::widgets::basic
{
	auto usePress(const PressOptions& opts) -> PressResult
	{
		auto [state, setState] = useState(PressState{});
		auto& armed = useRef(false);

		auto disabled = opts.Disabled;
		auto onPress = opts.OnPress;

		return {
			.State = state,
			.Events = {
				.OnPointerEnter = [=](auto&) -> auto
				{
					if (!disabled && !state.Hovered)
					{
						setState({.Hovered = true, .Pressed = state.Pressed});
					}
				},
				.OnPointerLeave = [&armed, setState](auto&) -> auto
				{
					armed = false;
					setState(PressState{});
				},
				.OnPointerDown = [&armed, setState, disabled](auto&) -> auto
				{
					if (disabled)
					{
						return;
					}
					armed = true;
					setState({.Hovered = true, .Pressed = true});
				},
				.OnPointerUp = [&armed, setState, disabled, onPress, state](
								   auto& ev
							   ) -> auto
				{
					bool fire = armed && !disabled;
					armed = false;
					setState({.Hovered = state.Hovered, .Pressed = false});
					if (fire && onPress)
					{
						onPress(ev);
					}
				},
			},
		};
	}

	auto buttonImpl(const ButtonProps& props) -> Element
	{
		auto press = usePress(
			{.Disabled = props.Disabled, .OnPress = props.OnPress}
		);

		styles::StyleList base;

		if (props.Disabled)
		{
			base.Add(styles::CursorShape(CursorShapeType::NotAllowed));
		}
		else
		{
			base.Add(styles::CursorShape(CursorShapeType::Pointer));
		}

		return box({
			.Style = props.Style + base,
			.Events = press.Events,
			.Children = props.Children,
		});
	}

	auto button(ButtonProps props) -> Element
	{
		return component(buttonImpl, std::move(props));
	}
}