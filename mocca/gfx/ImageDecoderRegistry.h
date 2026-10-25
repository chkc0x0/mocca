#pragma once
#include "gfx/ImageDecoder.h"
#include <cstddef>
#include <memory>
#include <span>
#include <string>
#include <vector>

namespace mocca::gfx
{
	class ImageDecoderRegistry
	{
	public:
		static void Register(std::unique_ptr<ImageDecoder> decoder);

		[[nodiscard]] static auto
		Probe(std::span<const std::byte> data, ImageInfo& out) -> bool;

		static auto Decode(std::span<const std::byte> data, DecodedImage& out)
			-> bool;
		
		static auto DecodeFile(const std::string& path, DecodedImage& out)
			-> bool;

	private:
		inline static std::vector<std::unique_ptr<ImageDecoder>> decoders;
	};
}
