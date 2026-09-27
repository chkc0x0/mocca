#pragma once
#include "Context.h"
#include "Logger.h"
#include "Surface.h"
#include <cctype>
#include <string>

namespace mocca
{
	enum class ApplicationEvent : uint64_t
	{
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
		SurfaceDestroyed = 0xf9580567ec3be68b
	};

	struct ApplicationID
	{
	public:
		ApplicationID(const std::string& id);
		ApplicationID() : _id("<not set>") {};
		ApplicationID(const ApplicationID&) = delete;
		ApplicationID(ApplicationID&&) noexcept = default;
		auto operator=(const ApplicationID&) -> ApplicationID& = delete;
		auto operator=(ApplicationID&&) noexcept -> ApplicationID& = default;

		// just found out you cant put it in a cpp file
		// lololol
		static constexpr auto ValidateID(std::string_view id) -> bool
		{
			if (id.empty())
			{
				return false;
			}

			int segments = 0;
			int segmentLen = 0;

			for (char c : id)
			{
				if (c == '.')
				{
					if (segmentLen == 0)
					{
						return false;
					}
					segments++;
					segmentLen = 0;
				}
				else if (
					(std::isalnum((unsigned char)c) != 0) || c == '_' || c == '-'
				)
				{
					segmentLen++;
				}
				else
				{
					return false;
				}
			}

			if (segmentLen == 0)
			{
				return false;
			}
			segments++;

			return segments == 2 || segments == 3;
		}
		[[nodiscard]] auto GetCompoundID() const -> std::string;

		[[nodiscard]] auto Name() const -> std::string_view;
		[[nodiscard]] auto Organization() const -> std::string_view;
		[[nodiscard]] auto Domain() const -> std::string_view;

	private:
		std::string _id;
		std::size_t _firstDot = std::string_view::npos;
		std::size_t _lastDot = std::string_view::npos;
	};

	class Application
	{
	public:
		inline static Application* main{nullptr};

		Application(const std::string& appId);
		~Application();

		void Tick(double dt);
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
		auto IsRunning() -> bool
		{
			return _surfaces.size() != 0;
		}

	private:
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
		uint8_t _stateOnEffectStreak = 0;
		bool _stateOnEffectWarned = false;

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