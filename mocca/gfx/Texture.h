#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>

namespace mocca::gfx
{
	struct TextureId
	{
	public:
		uint32_t Id;
		uint32_t Gen;

		auto operator==(const TextureId&) const -> bool = default;
	};

	struct TextureIdHash
	{
		auto operator()(const TextureId& t) const -> std::size_t
		{
			return (static_cast<std::size_t>(t.Id) << 32) ^ t.Gen;
		}
	};

	enum class TextureFormat : uint8_t
	{
		RGBA8,
		RGB8,
		Alpha8,
	};

	inline auto bytesPerPixel(TextureFormat format) -> uint32_t
	{
		switch (format)
		{
		case TextureFormat::RGBA8:
			return 4;
		case TextureFormat::RGB8:
			return 3;
		case TextureFormat::Alpha8:
			return 1;
		}

		return 0;
	}

	namespace TextureFlags
	{
		inline constexpr uint32_t RetainCPU = 1 << 0;
		inline constexpr uint32_t Shared = 1 << 1;
	}

	struct TextureDesc
	{
	public:
		uint32_t Width = 0;
		uint32_t Height = 0;
		TextureFormat Format = TextureFormat::RGBA8;
		uint32_t Flags = 0;

		auto operator==(const TextureDesc&) const -> bool = default;
	};

	struct Texture
	{
	public:
		TextureId Id{};
		TextureDesc Desc{};
		uint32_t Revision = 0;
		std::vector<std::byte> Pixels;
	};
}
