#pragma once
#include <cstdint>

namespace mocca::gfx
{
	// common apis
	enum class GraphicsApi : uint8_t
	{
		OpenGL,
		Vulkan,
		Software,
	};

	namespace GraphicsFlags
	{
		inline constexpr uint32_t Core = 1 << 0;
		inline constexpr uint32_t Share = 1 << 1;
	}

	struct GraphicsRequirements
	{
	public:
		GraphicsApi Api;
		uint8_t Major;
		uint8_t Minor;
		uint32_t Flags;
	};

	struct GraphicsHandle
	{
	public:
		GraphicsApi Api;
		void* (*GetProcAddress)(const char*);
		void* Native;
	};
}