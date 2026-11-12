#pragma once
#include "a11y/Snapshot.h"
#include "gfx/GfxTypes.h"
#include "input/InputTypes.h"
#include "utils/Math.h"
#include <memory>
#include <optional>

namespace mocca
{
	class Surface;

	class PlatformSurface
	{
	public:
		virtual ~PlatformSurface() = default;

		virtual void CollectEvents(InputBatch& batch) = 0;
		virtual auto ShouldClose() -> bool = 0;

		virtual auto Size() -> IVec2 = 0;
		virtual auto FramebufferSize() -> IVec2 = 0;
		virtual auto Scale() -> Vector2 = 0; // dpi. which i learned is actually dpr

		virtual void BeginFrame() = 0;
		virtual void EndFrame() = 0;

		virtual auto GfxHandle() -> gfx::GraphicsHandle = 0;

		virtual void SetCursor(CursorShapeType shape)
		{
		}

		virtual void SetImeCursorRect(Rectangle pos)
		{
		}

		virtual auto CreateA11yBridge(Surface& owner)
			-> std::unique_ptr<aria::Bridge>
		{
			return nullptr;
		}

		virtual auto A11yRootBounds() -> std::optional<aria::RootBounds>
		{
			return std::nullopt;
		}
	};
}
