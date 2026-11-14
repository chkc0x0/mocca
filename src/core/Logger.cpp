#include "core/Application.h"
#include <csignal>
#include <cstdio>
#include <print>

namespace mocca
{
	void Logger::Log(LogMessage& message)
	{
		if (callback == nullptr)
		{
			if (message.Severity == LogLevel::Error)
			{
				std::fprintf(
					stderr,
					"err (%.*s:%d): %.*s\nif you see this, it means an error "
					"occurred but you havent set up a log callback for mocca",
					static_cast<int>(message.File.size()),
					message.File.data(),
					message.Line,
					static_cast<int>(message.Message.size()),
					message.Message.data()
				);
				goto fail;
			}
			return;
		}

		callback(message, logUserData);
		lastError = message.Code;

	fail:
		if (message.Code == ErrorCode::AssertFailed)
		{
#ifdef DEBUG
#	ifdef _WIN32
			__debugbreak();
#	elifdef __clang__
			__builtin_debugtrap();
#	elif defined(__unix__) || defined(__APPLE__)
			std::raise(SIGTRAP);
#	else
			((void)0);
#	endif
#else
			abort();
#endif
		}
	}

	void Application::SetDefaultLogCallback()
	{
		Logger::SetLogCallback(
			[](const LogMessage& message, void* user) -> void
			{
				char buf[9];
				auto tm = time(nullptr);
				buf[strftime(buf, sizeof(buf), "%H:%M:%S", localtime(&tm))] =
					'\0';

				const char* severityStr = nullptr;

				switch (message.Severity)
				{
				case LogLevel::Debug:
					severityStr = "debug";
					break;
				case LogLevel::Info:
					severityStr = "info";
					break;
				case LogLevel::Warning:
					severityStr = "warning";
					break;
				case LogLevel::Error:
					severityStr = "error";
					break;
				}

				std::println(
					stderr,
					"{} {} ({:.{}}:{}): {}",
					buf,
					severityStr,
					message.File.data(),
					(int)message.File.size(),
					message.Line,
					std::string(message.Message)
				);
			}
		);
	}

	void Logger::LogImpl(
		LogLevel severity,
		std::string_view message,
		std::string_view file,
		std::string_view function,
		int line,
		ErrorCode code,
		std::format_args args
	)
	{
		auto msg = std::vformat(message, args);
		Log(severity, msg, file, function, line, code);
	}

	void Logger::LogImpl(
		LogLevel severity,
		std::string_view message,
		std::string_view file,
		std::string_view function,
		int line,
		ErrorCode code
	)
	{
		LogMessage logMessage{
			.Severity = severity,
			.Message = message,
			.File = file,
			.Function = function,
			.Line = line,
			.Code = code
		};
		Log(logMessage);
	}
}