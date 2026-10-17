#pragma once
#include "style/FontFamilyType.h"
#include "style/TransitionType.h"
#include "utils/Interner.h"

namespace mocca::detail
{
	struct TypeInterners
	{
	public:
		Interner<styles::TransitionType> Transitions;
		Interner<styles::FontFamilyType> FontFamilies;
	};
}