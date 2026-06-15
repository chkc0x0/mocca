#pragma once
#include "Context.h"
#include "Logger.h"
#include "Surface.h"
#include <format>
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
		auto GetCompoundID() const -> std::string;

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

		void SetDefaultLogCallback();

		void SetLogCallback(LogCallback callback, void* userData = nullptr)
		{
			_logger.Callback = std::move(callback);
			_logger.LogUserData = userData;
		}

		template <typename T, typename... Args>
		auto RegisterSurface(const SurfaceDesc& desc, Args&&... args) -> T*
		{
			auto surface = std::make_unique<T>(desc, args...);
			auto ptr = surface.get();
			_surfaces.push_back(std::move(surface));
			return ptr;
		}

		static void Log(LogMessage& message);

		// you're not really intended to use this
		// use the macros instead
		template <typename... Args>
		static void Log(LogLevel severity, std::string_view message,
						std::string_view file, std::string_view function,
						int line, ErrorCode code, Args&&... args)
		{
			if (Application::main == nullptr ||
				Application::main->_logger.Callback == nullptr)
			{
				return;
			}

			auto msg = std::vformat(message, std::make_format_args(args...));
			LogMessage logMessage{.Severity = severity,
								  .Message = msg,
								  .File = file,
								  .Function = function,
								  .Line = line,
								  .Code = code};

			Log(logMessage);
		}

		void Print();

	private:
		ApplicationID _id;

		struct
		{
			LogCallback Callback{nullptr};
			void* LogUserData{nullptr};
			ErrorCode LastError{ErrorCode::None};
		} _logger;

		std::vector<std::unique_ptr<Surface>> _surfaces;
		Context _context;
		friend auto getCtx() -> Context*;
	};
}