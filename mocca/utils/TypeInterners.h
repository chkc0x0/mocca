#pragma once
#include "gfx/Texture.h"
#include "style/FontStyleTypes.h"
#include "style/TransitionType.h"
#include "utils/Interner.h"

namespace mocca::detail
{
	struct TypeInterners
	{
	public:
		Interner<styles::TransitionType> Transitions;
		Interner<styles::FontFamilyType> FontFamilies;
		Interner<styles::FontVariationsType> FontVariations;
		Interner<styles::FontFeaturesType> FontFeatures;
		Interner<gfx::TextureId> BackgroundImages;
	};
}