#pragma once
#include "Canvas.h"
#include "Detail.h"
#include "Element.h"
#include "InputTypes.h"
#include "PlatformSurface.h"
#include <memory>

namespace mocca
{
	class Surface;

	enum class SurfaceFlags : char
	{
		None = 0,
		External = 1 << 0,
		AutomaticSize = 1 << 1
	};

	enum class SurfaceState : char
	{
		Alive,
		Zombie,
		Dead
	};

	struct SurfaceDesc
	{
		float Width;
		float Height;
		float X = -1;
		float Y = -1;
		std::string Title;
		SurfaceFlags Flags = SurfaceFlags::None;
		Surface* Parent = nullptr;
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

		void Update(double dt);
		void Tick(double dt);
		void Paint();
		void Print(int depth = 0) const;

		void MarkDirty()
		{
			_dirty = true;
			if (_desc.Parent != nullptr && !IsPlatformBacked())
			{
				_desc.Parent->MarkDirty();
			}
		}

		[[nodiscard]] auto GetDrawData() const
			-> const std::vector<cmds::DrawCommand>&
		{
			return _canvas.Commands();
		}

		[[nodiscard]] auto GetDescriptor() const -> SurfaceDesc
		{
			return _desc;
		}

		template <typename T> void SetPlatform()
		{
			// i cannot simplify this further alright?
			mc_assert(
				_desc.Parent == nullptr || IsExternal(),
				"cant set a platform for a composite surface"
			);
			mc_assert(_platform == nullptr, "this surface already has a platform");
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

		[[nodiscard]] auto GetState() const -> SurfaceState
		{
			return _state;
		}

		[[nodiscard]] auto GetParent() const -> Surface*
		{
			return _desc.Parent;
		}

		void RequestClose();
		void ForceDestroy()
		{
			_state = SurfaceState::Dead;
		}

		void SetZombieTimeout(int frames)
		{
			_zombieTimeout = frames;
		}

		[[nodiscard]] auto IsDirty() const -> bool
		{
			return _dirty;
		}

		[[nodiscard]] auto Tree() const -> detail::Node*
		{
			return _root.get();
		}

		[[nodiscard]] auto FindSurfaceContaining(detail::NodeId id) -> Surface*;

		void ProcessInput(const InputBatch& batch);

		void FocusNode(detail::NodeId id);
		void ClearFocus();
		[[nodiscard]] auto GetFocusedNode() const -> detail::NodeId
		{
			return _focusedNode;
		}

		void CapturePointer(detail::NodeId id);
		void ReleasePointer();
		[[nodiscard]] auto GetCapturedNode() const -> detail::NodeId
		{
			return _capturedNode;
		}

		// this surface (or a subsurface) is capturing
		[[nodiscard]] auto IsCapturing() const -> bool
		{
			return _capturedNode != 0 || _captureSurface != nullptr;
		}

		void OnOutsidePress(
			const std::function<bool(Surface&, const PointerEvent&)>& cb
		)
		{
			_onOutsidePress = cb;
		}

	protected:
		void RoutePointer(PointerEvent ev);
		void RouteKey(KeyEvent& ev);
		void RouteText(TextEvent& ev);
		auto NotifyOutsidePress(const PointerEvent& ev) -> bool;

	private:
		SurfaceDesc _desc;
		std::unique_ptr<PlatformSurface> _platform;
		detail::NodeId _rootId = 0;
		std::unique_ptr<detail::Node> _root = nullptr;
		SurfaceState _state = SurfaceState::Alive;
		bool _dirty = true;
		int _zombieTimeout = 0;
		Canvas _canvas;

		std::vector<std::unique_ptr<Surface>> _children;

		detail::NodeId _focusedNode = 0;
		detail::NodeId _capturedNode = 0;

		// DAMN YOU STD FUNCTION!!!!
		std::function<bool(Surface&, const PointerEvent&)> _onOutsidePress;

		Surface* _focusSurface = nullptr;
		Surface* _captureSurface = nullptr;

		friend class Application;
	};
}