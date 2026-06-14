#include "Application.h"

namespace mocca
{
	void Application::Log(LogMessage& message)
	{
		if (Application::main == nullptr ||
			Application::main->_logger.Callback == nullptr)
		{
			return;
		}

		Application::main->_logger.Callback(
			message, Application::main->_logger.LogUserData);
		Application::main->_logger.LastError = message.Code;
	}

	void Application::SetDefaultLogCallback()
	{
		SetLogCallback(
			[](const LogMessage& message, void* user)
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

				std::fprintf(stderr, "%s %s (%.*s:%d): %s\n", buf, severityStr,
							 (int)message.File.size(), message.File.data(),
							 message.Line,
							 std::string(message.Message).c_str());
			});
	}
}