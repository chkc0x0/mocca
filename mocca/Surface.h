#pragma once
#include "Canvas.h"
#include "Detail.h"
#include "Element.h"
#include <memory>

namespace mocca
{
	enum class SurfaceFlags : char
	{
		None = 1 << 0,
		External = 1 << 1
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

		virtual ~Surface() = default;

		void Tick(double dt);
		void Paint();
		void Print(int depth = 0);

		virtual void Update() {};

		virtual auto IsRunning() -> bool
		{
			return true;
		}

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

		void UserHandle(void* handle)
		{
			_userHandle = handle;
		}

		auto UserHandle() -> void*
		{
			return _userHandle;
		}

		[[nodiscard]] auto IsDirty() const -> bool
		{
			return _dirty;
		}

		[[nodiscard]] auto Tree() const -> detail::Node*
		{
			return _root.get();
		}

		[[nodiscard]] auto IsExternal() const -> bool
		{
			return _desc.Flags == SurfaceFlags::External;
		}

	private:
		SurfaceDesc _desc;
		std::unique_ptr<detail::Node> _root = nullptr;
		bool _dirty = true;
		Canvas _canvas;
		void* _userHandle;
	};
}