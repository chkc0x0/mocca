#pragma once
#include "core/Identifiers.h"
#include "gfx/Texture.h"
#include <cstdint>
#include <initializer_list>
#include <string_view>
#include <vector>

namespace mocca::styles
{
	struct BackgroundImageEntry
	{
	public:
		gfx::TextureId Texture{};

		BackgroundImageEntry() = default;

		BackgroundImageEntry(gfx::TextureId texture) : Texture(texture)
		{
		}

		BackgroundImageEntry(const char* path, uint32_t flags = 0);
		BackgroundImageEntry(std::string_view path, uint32_t flags = 0);

		auto operator==(const BackgroundImageEntry&) const -> bool = default;
	};

	struct BackgroundImageType
	{
	public:
		std::vector<BackgroundImageEntry> Layers;

		BackgroundImageType() = default;

		BackgroundImageType(std::initializer_list<BackgroundImageEntry> layers)
			: Layers(layers)
		{
		}

		auto operator==(const BackgroundImageType&) const -> bool = default;
	};
}

namespace std
{
	template <> struct hash<mocca::styles::BackgroundImageEntry>
	{
		auto operator()(const mocca::styles::BackgroundImageEntry& e)
			const noexcept -> size_t
		{
			size_t h = 0;
			mocca::detail::hashCombine(h, std::hash<uint32_t>{}(e.Texture.Id));
			mocca::detail::hashCombine(h, std::hash<uint32_t>{}(e.Texture.Gen));
			return h;
		}
	};

	template <> struct hash<mocca::styles::BackgroundImageType>
	{
		auto operator()(const mocca::styles::BackgroundImageType& v)
			const noexcept -> size_t
		{
			size_t h = std::hash<size_t>{}(v.Layers.size());
			for (const auto& layer : v.Layers)
			{
				mocca::detail::hashCombine(
					h,
					std::hash<mocca::styles::BackgroundImageEntry>{}(layer)
				);
			}
			return h;
		}
	};
}
