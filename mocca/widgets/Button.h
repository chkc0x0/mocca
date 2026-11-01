#pragma once
#include "core/Element.h"
#include "style/StyleList.h"
#include "widgets/ComponentTypes.h"
#include <vector>

namespace mocca::widgets::basic
{
	struct ButtonProps
	{
		ComponentSize Size = ComponentSize::Default;
		ButtonState State = ButtonState::Default;
		bool Disabled = false;
		std::vector<Element> Children;
		std::function<void(PointerEvent&)> OnPress;
		styles::StyleList Style;
	};

	struct PressOptions
	{
		bool Disabled = false;
		std::function<void(PointerEvent&)> OnPress;
	};

	struct PressState
	{
		bool Hovered = false;
		bool Pressed = false;
	};

	struct PressResult
	{
		PressState State;
		EventHandlers Events;
	};

	auto button(ButtonProps props) -> Element;
}