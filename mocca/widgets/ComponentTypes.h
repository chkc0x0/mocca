#pragma once

namespace mocca::widgets
{
	enum class ComponentSize : char
	{
		XS,
		SM,
		Default,
		LG,
		XL,
	};

	enum class ButtonState : char
	{
		Default,
		Outline,
		Ghost,
		Destructive,
	};
}