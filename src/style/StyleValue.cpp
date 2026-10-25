#include "style/StyleValue.h"
#include "core/Application.h"
#include "utils/TypeInterners.h"

namespace mocca::styles::detail
{
	auto internTransition(const TransitionType& type) -> uint32_t
	{
		return Application::main->GetInterners()->Transitions.Intern(type);
	}

	auto resolveTransition(uint32_t ref) -> const TransitionType&
	{
		return Application::main->GetInterners()->Transitions.Get(ref);
	}

	auto internFontFamily(const FontFamilyType& family) -> uint32_t
	{
		return Application::main->GetInterners()->FontFamilies.Intern(family);
	}

	auto resolveFontFamily(uint32_t ref) -> const FontFamilyType&
	{
		return Application::main->GetInterners()->FontFamilies.Get(ref);
	}

	auto internFontVariations(const FontVariationsType& variations) -> uint32_t
	{
		return Application::main->GetInterners()->FontVariations.Intern(
			variations
		);
	}

	auto resolveFontVariations(uint32_t ref) -> const FontVariationsType&
	{
		return Application::main->GetInterners()->FontVariations.Get(ref);
	}

	auto internFontFeatures(const FontFeaturesType& features) -> uint32_t
	{
		return Application::main->GetInterners()->FontFeatures.Intern(features);
	}

	auto resolveFontFeatures(uint32_t ref) -> const FontFeaturesType&
	{
		return Application::main->GetInterners()->FontFeatures.Get(ref);
	}

	auto internBackgroundImage(const gfx::TextureId& id) -> uint32_t
	{
		return Application::main->GetInterners()->BackgroundImages.Intern(id);
	}

	auto resolveBackgroundImage(uint32_t ref) -> const gfx::TextureId&
	{
		return Application::main->GetInterners()->BackgroundImages.Get(ref);
	}
}