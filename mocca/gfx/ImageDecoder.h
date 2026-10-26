#pragma once
#include "gfx/Texture.h"
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

namespace mocca::gfx
{
	struct ImageInfo
	{
		uint32_t Width = 0;
		uint32_t Height = 0;
	};

	struct DecodedImage
	{
		uint32_t Width = 0;
		uint32_t Height = 0;
		TextureFormat Format = TextureFormat::RGBA8;
		PixelBuffer Pixels;
	};

	class ImageDecoder
	{
	public:
		virtual ~ImageDecoder() = default;

		[[nodiscard]] virtual auto Name() const -> std::string_view = 0;

		[[nodiscard]] virtual auto Probe(
			std::span<const std::byte> data,
			ImageInfo& out
		) const -> bool = 0;

		virtual auto Decode(std::span<const std::byte> data, DecodedImage& out)
			const -> bool = 0;
	};
}
