#pragma once
#include "platform/PlatformInterface.h"

namespace sandbox
{
	class GLFWPlatformClipboard : public mocca::platform::PlatformClipboard
	{
	public:
		auto Formats() -> std::vector<std::string> override;
		auto Get(std::string_view mime)
			-> std::optional<std::vector<uint8_t>> override;
		void Set(
			const std::vector<mocca::platform::ClipboardEntry>& offers
		) override;
	};

	class GLFWPlatformInterface : public mocca::platform::PlatformInterface
	{
	public:
		GLFWPlatformInterface();
		~GLFWPlatformInterface() override;

		void PumpEvents() override;
		void WaitEvents() override;
		void Wake() override;

		auto CreateSurface(
			const mocca::SurfaceDesc& desc,
			const mocca::gfx::GraphicsRequirements& requirements
		) -> std::unique_ptr<mocca::PlatformSurface> override;
	};

	auto createGLFWPlatformInterface()
		-> std::unique_ptr<mocca::platform::PlatformInterface>;
}