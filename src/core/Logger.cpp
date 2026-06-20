#include "Application.h"
#include <print>

namespace mocca
{
	void Logger::Log(LogMessage& message)
	{
		if (callback == nullptr)
		{
			return;
		}

		callback(message, logUserData);
		lastError = message.Code;
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
}