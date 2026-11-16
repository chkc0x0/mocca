#pragma once
#include "core/ElementEvents.h"
#include "core/NodeRef.h"
#include "input/InputTypes.h"
#include <functional>

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

	struct PressOptions
	{
		NodeRef* Ref = nullptr;
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
		EventView Events;
	};

	auto usePress(const PressOptions& opts) -> PressResult;
}