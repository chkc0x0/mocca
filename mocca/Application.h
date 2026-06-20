#pragma once
#include "Context.h"
#include "Logger.h"
#include "Surface.h"
#include <string>
#include <utility>

namespace mocca
{
	struct ApplicationID
	{
	public:
		ApplicationID(const std::string& id);
		ApplicationID() : _id("<not set>") {};

		static constexpr auto ValidateID(std::string_view id) -> bool;
		[[nodiscard]] auto GetCompoundID() const -> std::string;

		std::string_view Name;
		std::string_view Organization;
		std::string_view Domain;

	private:
		std::string _id;
	};

	class Application
	{
	public:
		inline static Application* main{nullptr};

		Application(const std::string& appId);
		~Application();

		void Tick(double dt);
		void DumpTree();

		static auto GetAppID() -> ApplicationID
		{
			if (main == nullptr)
			{
				return {};
			}

			return main->_id;
		}

		static void SetDefaultLogCallback();

		template <typename T, typename... Args>
		auto RegisterSurface(const SurfaceDesc& desc, Args&&... args) -> T*
		{
			auto surface = std::make_unique<T>(desc, args...);
			auto ptr = surface.get();
			_surfaces.push_back(std::move(surface));
			return ptr;
		}

		void Print();
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

		// dead surfaces live +1 frame so code after tick doesnt uaf.
		// this is quite hacky and it'll probably break someday,
		// assumes nothing holds a ref to surfaces for >1 frame
		std::vector<std::unique_ptr<Surface>> _deadSurfaces;

		Context _context;
		friend auto getCtx() -> Context*;
	};
}