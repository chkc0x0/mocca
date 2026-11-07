#pragma once
#include "gfx/Canvas.h"
#include "core/Node.h"
#include "Element.h"
#include "gfx/RenderDevice.h"
#include "gfx/Texture.h"
#include "input/InputTypes.h"
#include "platform/PlatformSurface.h"
#include <algorithm>
#include <memory>
#include <unordered_map>
#include <unordered_set>

namespace mocca
{
	class Surface;

	namespace gfx
	{
		class TextureRegistry;
	}

	namespace SurfaceFlags
	{
		using FlagType = uint64_t;

		constexpr FlagType None = 0;
		constexpr FlagType External = 1 << 0;
		constexpr FlagType AutomaticSize = 1 << 1;
	};

	enum class SurfaceState : char
	{
		Alive,
		Zombie,
		Dead,
	};

	struct SurfaceDesc
	{
		float Width;
		float Height;
		float X = -1;
		float Y = -1;
		std::string Title;
		uint64_t Flags = SurfaceFlags::None;
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

		void UpdateTree(double dt);
		void Update(double dt);
		void Tick(double dt);
		void Paint();
		void SyncTextureConsumers(gfx::TextureRegistry& registry);
		void Print(int depth = 0) const;

		void MarkDirty()
		{
			_dirty = true;
			if (_desc.Parent != nullptr && !IsPlatformBacked())
			{
				_desc.Parent->MarkDirty();
			}
		}

		void MarkComponentDirty(detail::NodeId id)
		{
			if (std::find(
					_dirtyComponents.begin(), _dirtyComponents.end(), id
				) == _dirtyComponents.end())
			{
				_dirtyComponents.push_back(id);
			}
		}

		void MarkAnimating()
		{
			_animating = true;
		}

		void RequestRepaint()
		{
			_repaint = true;
			if (_desc.Parent != nullptr && !IsPlatformBacked())
			{
				_desc.Parent->RequestRepaint();
			}
		}

		auto DetachChild(Surface* which) -> std::unique_ptr<Surface>;

		[[nodiscard]] auto GetDrawData() const
			-> std::span<const cmds::DrawCommand>
		{
			return _canvas.Commands();
		}

		[[nodiscard]] auto GetDescriptor() const -> SurfaceDesc
		{
			return _desc;
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
			if (_state == SurfaceState::Zombie)
			{
				_zombieRemaining = _zombieTimeout;
			}
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

		void FocusNode(
			detail::NodeId id,
			FocusReason reason = FocusReason::Programmatic,
			bool force = false
		);
		void ClearFocus(FocusReason reason = FocusReason::Programmatic);
		void FocusNext(bool backward = false);
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

		[[nodiscard]] auto GetClipboardText() -> std::string;
		void SetClipboardText(std::string_view text);
		void SetImeCursorRect(Rectangle rect);

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

		void RouteKey(KeyEvent& ev);
		void RouteText(TextEvent& ev);

	protected:
		void RoutePointer(PointerEvent ev);
		void UpdateHover(detail::Node* hit, const PointerEvent& ev);
		auto NotifyOutsidePress(const PointerEvent& ev) -> bool;

	private:
		void _sweepChildren();
		void _updateDirtyComponents(
			const std::vector<detail::NodeId>& components,
			double dt
		);
		void _updateCursor(detail::Node* node);
		void _advanceAnimations(double dt);
		static void AnnounceSubtreeDestroyed(Surface* s);

		SurfaceDesc _desc;
		std::unique_ptr<PlatformSurface> _platform;
		detail::NodeId _rootId = 0;
		std::unique_ptr<detail::Node> _root = nullptr;
		SurfaceState _state = SurfaceState::Alive;
		std::vector<detail::NodeId> _dirtyComponents;
		bool _dirty = true;
		bool _animating = false;
		bool _repaint = true;
		bool _relayout = false;
		CursorShapeType _cursor = CursorShapeType::Default;

		int _zombieTimeout = 0;
		int _zombieRemaining = 0;
		Canvas _canvas;

		std::unordered_map<gfx::TextureId, int, gfx::TextureIdHash> _textureAge;
		std::unordered_set<gfx::TextureId, gfx::TextureIdHash> _textureSeen;

		std::vector<std::unique_ptr<Surface>> _children;

		detail::NodeId _focusedNode = 0;
		detail::NodeId _capturedNode = 0;
		detail::NodeId _hoveredNode = 0;

		std::function<bool(Surface&, const PointerEvent&)> _onOutsidePress;

		Surface* _focusSurface = nullptr;
		Surface* _captureSurface = nullptr;

		std::unique_ptr<gfx::Renderer> _renderer;

		friend class Application;
	};
}