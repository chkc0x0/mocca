#pragma once
#include "gfx/GfxTypes.h"
#include "input/InputTypes.h"
#include "utils/Math.h"

namespace mocca
{
	class PlatformSurface
	{
	public:
		virtual ~PlatformSurface() = default;

		virtual void CollectEvents(InputBatch& batch) = 0;
		virtual auto ShouldClose() -> bool = 0;

		virtual auto Size() -> IVec2 = 0;
		virtual auto FramebufferSize() -> IVec2 = 0;
		virtual auto Scale() -> Vector2 = 0; // dpi

		virtual void BeginFrame() = 0;
		virtual void EndFrame() = 0;

		virtual auto GfxHandle() -> gfx::GraphicsHandle = 0;
	};
}
