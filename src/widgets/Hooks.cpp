#include "core/Hooks.h"
#include "widgets/ComponentTypes.h"

namespace mocca::widgets
{
	auto usePress(const PressOptions& opts) -> PressResult
	{
		auto [state, setState] = useState(PressState{});
		auto armed = useRef(false);
		auto* node = opts.Ref;

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
							   [](auto val) -> PressState
							   {
								   return {
									   .Hovered = true,
									   .Pressed = val.Pressed,
								   };
							   }
						   );
					   }
				   }),
				on(events::PointerLeave,
				   [armed, setState](PointerEvent&) -> auto
				   {
					   armed.Current = false;
					   setState(
						   [](auto val) -> PressState
						   {
							   return {};
						   }
					   );
				   }),
				on(events::PointerDown,
				   [armed, setState, disabled](PointerEvent&) -> auto
				   {
					   if (disabled)
					   {
						   return;
					   }
					   armed.Current = true;
					   setState(
						   [](auto val) -> PressState
						   {
							   return {
								   .Hovered = true,
								   .Pressed = true,
							   };
						   }
					   );
				   }),
				on(events::PointerUp,
				   [armed, setState, disabled, onPress](PointerEvent& ev)
					   -> auto
				   {
					   bool fire = armed.Current && !disabled;
					   armed.Current = false;
					   setState(
						   [](auto val) -> PressState
						   {
							   return {
								   .Hovered = val.Hovered,
								   .Pressed = false,
							   };
						   }
					   );
					   if (fire && onPress)
					   {
						   onPress(ev);
					   }
				   }),
				on(events::Activate,
				   [disabled, onPress, node](ActivateEvent&) -> void
				   {
					   if (disabled || !onPress)
					   {
						   return;
					   }

					   PointerEvent synth{};
					   synth.EventType = PointerEvent::Type::Up;
					   if (node)
					   {
						   auto r = node->Rect();
						   synth.X = r.X + (r.Width * 0.5F);
						   synth.Y = r.Y + (r.Height * 0.5F);
					   }
					   onPress(synth);
				   }),
			},
		};
	}
}