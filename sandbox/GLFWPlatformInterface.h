#pragma once
#include "platform/PlatformInterface.h"

namespace sandbox
{
	class GLFWPlatformInterface : public mocca::platform::PlatformInterface
	{
	public:
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