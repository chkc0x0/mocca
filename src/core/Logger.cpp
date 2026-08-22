#include "Application.h"
#include <csignal>
#include <print>

namespace mocca
{
	void Logger::Log(LogMessage& message)
	{
		if (callback == nullptr)
		{
			if (message.Severity == LogLevel::Error)
			{
				std::println(
					stderr,
					"err ({:.{}}:{}): {}\nif you see this, it means an error "
					"occurred but you havent set up a log callback for mocca",
					message.File.data(),
					(int)message.File.size(),
					message.Line,
					std::string(message.Message)
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
#	if defined(_MSC_VER)
			__debugbreak();
#	elif defined(__clang__)
			__builtin_debugtrap();
#	elif defined(__GNUC__)
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
		LogMessage logMessage{
			.Severity = severity,
			.Message = msg,
			.File = file,
			.Function = function,
			.Line = line,
			.Code = code
		};
		Log(logMessage);
	}
}