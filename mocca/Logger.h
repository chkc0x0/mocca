#pragma once
#include <functional>
#include <string_view>

namespace mocca
{
	enum class LogLevel : char
	{
		Debug,
		Info,
		Warning,
		Error,
	};

	enum class ErrorCode : unsigned char
	{
		None = 0,
		AssertFailed = 1,
		InvalidState = 2,
		InvalidArgument = 3,
		Unknown = 255
	};

	struct LogMessage
	{
	public:
		LogLevel Severity;
		std::string_view Message;
		std::string_view File;
		std::string_view Function;
		int Line;
		ErrorCode Code;
	};

	using LogCallback = std::function<void(const LogMessage&, void* user)>;
}

#define mc_log(level, code, message, ...)                                      \
	mocca::Application::Log(level, message, __FILE__, __FUNCTION__, __LINE__,  \
							code, ##__VA_ARGS__)

#define mc_info(message, ...)                                                  \
	mc_log(mocca::LogLevel::Info, mocca::ErrorCode::None, message,             \
		   ##__VA_ARGS__)

#define mc_warning(message, ...)                                               \
	mc_log(mocca::LogLevel::Warning, mocca::ErrorCode::None, message,          \
		   ##__VA_ARGS__)

#define mc_error(code, message, ...)                                           \
	mc_log(mocca::LogLevel::Error, code, message, ##__VA_ARGS__)

#ifdef DEBUG
#	define mc_debug(message, ...)                                             \
		mc_log(mocca::LogLevel::Debug, mocca::ErrorCode::None, message,        \
			   ##__VA_ARGS__)
#else
#	define mc_debug(message, ...)
#endif

#define mc_assert(expr, message, ...)                                          \
	do                                                                         \
	{                                                                          \
		if (!(expr))                                                           \
		{                                                                      \
			mc_log(mocca::LogLevel::Error, mocca::ErrorCode::AssertFailed,     \
				   std::format("assertion \"{}\" failed: {}", #expr, message),   \
				   __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__);           \
		}                                                                      \
	} while (0)
