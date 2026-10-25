#pragma once
#include "gfx/ImageDecoder.h"

namespace mocca::gfx
{
	class StbImageDecoder : public ImageDecoder
	{
	public:
		[[nodiscard]] auto Name() const -> std::string_view override;
		[[nodiscard]] auto Probe(std::span<const std::byte> data, ImageInfo& out)
			const -> bool override;
		auto Decode(std::span<const std::byte> data, DecodedImage& out)
			const -> bool override;
	};
}
