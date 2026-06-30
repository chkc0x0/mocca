#pragma once
#include "Canvas.h"
#include "Detail.h"
#include "Element.h"
#include "InputTypes.h"
#include "PlatformSurface.h"
#include <memory>

namespace mocca
{
	enum class SurfaceFlags : char
	{
		None = 1 << 0,
		External = 1 << 1,
		AutomaticSize = 1 << 2
	};

	struct SurfaceDesc
	{
		int Width;
		int Height;
		int X = -1;
		int Y = -1;
		std::string Title;
		SurfaceFlags Flags = SurfaceFlags::None;
		ComponentFn Root;
	};

	class Surface
	{
	public:
		Surface(const SurfaceDesc& desc)
		{
			_desc = desc;
		}

		~Surface();

		void Tick(double dt);
		void Paint();
		void Print(int depth = 0) const;

		void MarkDirty()
		{
			_dirty = true;
		}

		auto GetDrawData() -> std::vector<cmds::DrawCommand>
		{
			return _canvas.Commands();
		}

		[[nodiscard]] auto GetDescriptor() const -> SurfaceDesc
		{
			return _desc;
		}

		template<typename T>
		void SetPlatform()
		{
			_platform = std::make_unique<T>(this, GetDescriptor());
		}

		[[nodiscard]] auto GetPlatform() const -> PlatformSurface*
		{
			return _platform.get();
		}

		[[nodiscard]] auto IsPlatformBacked() const -> bool
		{
			return _platform != nullptr;
		}

		[[nodiscard]] auto IsExternal() const -> bool
		{
			return (static_cast<int>(_desc.Flags) &
				   static_cast<int>(SurfaceFlags::External)) != 0;
		}

		[[nodiscard]] auto IsAutomaticSize() const -> bool
		{
			return (static_cast<int>(_desc.Flags) &
				   static_cast<int>(SurfaceFlags::AutomaticSize)) != 0;
		}

		void RequestClose()
		{
			_shouldClose = true;
		}

		[[nodiscard]] auto ShouldClose() const -> bool
		{
			return _shouldClose;
		}

		[[nodiscard]] auto IsDirty() const -> bool
		{
			return _dirty;
		}

		[[nodiscard]] auto Tree() const -> detail::Node*
		{
			return _root.get();
		}

		void ProcessInput(const InputBatch& batch);

	private:
		SurfaceDesc _desc;
		std::unique_ptr<PlatformSurface> _platform;
		std::unique_ptr<detail::Node> _root = nullptr;
		bool _dirty = true;
		bool _shouldClose = false;
		Canvas _canvas;
	};
}
