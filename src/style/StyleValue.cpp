#include "style/StyleValue.h"
#include "core/Application.h"
#include "utils/TypeInterners.h"

namespace mocca::styles
{
	BackgroundImageEntry::BackgroundImageEntry(const char* path, uint32_t flags)
		: BackgroundImageEntry(std::string_view(path), flags)
	{
	}

	BackgroundImageEntry::BackgroundImageEntry(
		std::string_view path,
		uint32_t flags
	) : Texture(detail::loadImageFile(path, flags))
	{
	}
}

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

	auto internBackgroundImage(const BackgroundImageType& image) -> uint32_t
	{
		return Application::main->GetInterners()->BackgroundImages.Intern(image);
	}

	auto resolveBackgroundImage(uint32_t ref) -> const BackgroundImageType&
	{
		return Application::main->GetInterners()->BackgroundImages.Get(ref);
	}

	auto loadImageFile(std::string_view path, uint32_t flags) -> gfx::TextureId
	{
		std::string key(path);
		return Application::main->Textures().LoadNamed(key, key, flags);
	}

	auto loadImageMemory(
		std::string_view name,
		std::span<const std::byte> data,
		uint32_t flags
	) -> gfx::TextureId
	{
		return Application::main->Textures().LoadNamed(std::string(name), data, flags);
	}
}