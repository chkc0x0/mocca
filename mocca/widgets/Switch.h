#pragma once
#include "core/Element.h"

namespace mocca::widgets
{
	struct SwitchProps
	{
		mc_baseComponentProps;
		std::optional<bool> Value = false;
		std::function<void(bool)> OnValueChange;
		bool DefaultValue = false;
		bool Disabled = false;
	};

	auto Switch(SwitchProps props) -> Element;
}