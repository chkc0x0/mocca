#pragma once
#include "a11y/A11y.h"
#include "style/BackgroundImageTypes.h"
#include "style/FontStyleTypes.h"
#include "style/Length.h"
#include "style/TransitionType.h"
#include "utils/Interner.h"

namespace mocca::detail
{
	struct TypeInterners
	{
	public:
		Interner<styles::TransitionList> Transitions;
		Interner<styles::FontFamilyType> FontFamilies;
		Interner<styles::FontVariationsType> FontVariations;
		Interner<styles::FontFeaturesType> FontFeatures;
		Interner<styles::BackgroundImageType> BackgroundImages;
		Interner<styles::Vector2Length> Vector2Lengths;
		A11yInterner A11y;
	};
}