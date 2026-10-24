#pragma once
#include "Context.h"
#include "Logger.h"
#include "Surface.h"
#include "gfx/RenderDevice.h"
#include "text/FontRegistry.h"
#include <chrono>
#include <cstddef>
#include <string>

namespace mocca
{
	namespace detail
	{
		struct TypeInterners;
	}

	enum class ApplicationEvent : uint64_t
	{
		// app.init (data = nullptr)
		// called in the first tick
		Init = 0x563f6467da5e005a,

		// app.shutdown (data = nullptr)
		// called at destructor
		Shutdown = 0x88bab3ab1b23a982,

		// app.poll (data = nullptr)
		// emitted every tick before input collection
		Poll = 0x2dc706262aa3f105,

		// surface.created (data = Surface*) / cancellable
		// called before adding surface to surface list
		SurfaceCreated = 0x1ef43f299f709040,

		// surface.closed (data = Surface*) / cancellable
		// called on RequestClose()
		SurfaceClosed = 0xe932d1695758b080,

		// surface.destroyed (data = Surface*)
		// called on ~Surface
		SurfaceDestroyed = 0xf9580567ec3be68b,
	};

	class Application
	{
	public:
		inline static Application* main{nullptr};

		Application(const std::string& appId);
		~Application();

		void Tick(double dt);
		void Tick();
		void DumpTree();

		static auto GetAppID() -> const ApplicationID&
		{
			mc_assert(
				main != nullptr,
				"expected to have an Application instance running"
			);

			return main->_id;
		}

		static void SetDefaultLogCallback();

		auto RegisterSurface(const SurfaceDesc& desc) -> Surface*;

		// return false in order to stop callback invocations
		// and/or reject an event (e.g: surface.closed)
		void On(
			ApplicationEvent event,
			std::function<bool(void*, void*)> cb,
			void* userData = nullptr
		);

		void On(
			std::string_view event,
			std::function<bool(void*, void*)> cb,
			void* userData = nullptr
		);

		auto EmitEvent(ApplicationEvent, void* data = nullptr) -> bool;
		auto EmitEvent(std::string_view event, void* data = nullptr) -> bool;
		void RemoveCallbacks(std::string_view event);

		void Print() const;
		auto IsRunning() const -> bool
		{
			return !_surfaces.empty();
		}

		auto GetInterners() const -> detail::TypeInterners*
		{
			return _interners.get();
		}

		auto Fonts() -> mtext::FontRegistry&
		{
			return _fonts;
		}

		auto GetRenderDevice() -> gfx::RenderDevice*
		{
			return _gfxDevice.get();
		}

		template <typename T, typename... Args>
		void SetRenderDevice(Args&&... args)
		{
			_gfxDevice =
				std::make_unique<T>(std::forward<Args>(args)...);
		}

	private:
		void _createPendingSurfaces();
		void _reparentPendingSurfaces();

		ApplicationID _id;

		struct
		{
			LogCallback Callback{nullptr};
			void* LogUserData{nullptr};
			ErrorCode LastError{ErrorCode::None};
		} _logger;

		std::vector<std::unique_ptr<Surface>> _surfaces;
		std::vector<Surface*> _platformSurfaces;
		std::vector<std::pair<Surface*, std::unique_ptr<Surface>>>
			_pendingSurfaces;
		std::vector<std::tuple<Surface*, Surface*, Surface*>> _pendingReparents;

		bool _inTick = false;
		bool _init = false;
		uint32_t _stateOnEffectStreak = 0;
		bool _stateOnEffectWarned = false;

		std::chrono::steady_clock::time_point _last;
		bool _hasLast = false;
		std::size_t _animationsActive = 0;

		std::unique_ptr<detail::TypeInterners> _interners;
		mtext::FontRegistry _fonts;
		std::unique_ptr<gfx::RenderDevice> _gfxDevice;

		struct
		{
			struct EventCallback
			{
			public:
				// void(void* data, void* user)
				std::function<bool(void*, void*)> Callback;
				void* User;
			};

			std::unordered_map<uint64_t, std::vector<EventCallback>> Events;

			struct PendingEventOp
			{
				uint64_t Hash;
				bool Clear;
				EventCallback Cb;
			};

			std::vector<PendingEventOp> PendingEvents;
			int EmitDepth = 0;
		} _events;

		void _drainPendingEvents();

		Context _context;

		friend class Surface;
		friend auto getCtx() -> Context*;
	};
}