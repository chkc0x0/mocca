#pragma once
#include "style/TransitionType.h"
#include "utils/Interner.h"

namespace mocca::detail
{
	struct TypeInterners
	{
	public:
		Interner<styles::TransitionType> Transitions;
	};
}