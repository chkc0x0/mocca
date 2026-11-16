#pragma once
#include "core/Element.h"

namespace mocca::widgets
{
	enum class ButtonVariant : char
	{
		Default,
		Secondary,
		Outline,
		Ghost,
		Destructive,
	};

	struct ButtonProps
	{
	public:
		mc_childComponentProps;
		ButtonVariant Variant = ButtonVariant::Default;
		bool Disabled = false;
		std::function<void(PointerEvent&)> OnPress;
	};

	auto button(ButtonProps props) -> Element;
	auto button(const std::string& text, std::function<void(PointerEvent&)> cb) -> Element;
}