#pragma once
#include "platform/PlatformInterface.h"
#include <cstdint>
#include <unordered_map>

union SDL_Event;

class SDL3PlatformSurface;

namespace sandbox
{
	class SDL3PlatformClipboard : public mocca::platform::PlatformClipboard
	{
	public:
		auto Formats() -> std::vector<std::string> override;
		auto Get(std::string_view mime)
			-> std::optional<std::vector<uint8_t>> override;
		void Set(
			const std::vector<mocca::platform::ClipboardEntry>& offers
		) override;
	};

	class SDL3PlatformInterface : public mocca::platform::PlatformInterface
	{
	public:
		SDL3PlatformInterface();
		~SDL3PlatformInterface() override;

		void PumpEvents() override;
		void WaitEvents() override;
		void Wake() override;

		auto CreateSurface(
			const mocca::SurfaceDesc& desc,
			const mocca::gfx::GraphicsRequirements& requirements
		) -> std::unique_ptr<mocca::PlatformSurface> override;

		void RegisterSurface(
			std::uint32_t windowId,
			::SDL3PlatformSurface* surface
		);
		void UnregisterSurface(std::uint32_t windowId);

	private:
		void _dispatch(const SDL_Event& event);

		std::unordered_map<std::uint32_t, ::SDL3PlatformSurface*> _surfaces;
	};

	auto createSDL3PlatformInterface()
		-> std::unique_ptr<mocca::platform::PlatformInterface>;
}
