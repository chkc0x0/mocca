#pragma once
#include <format>
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
		UserSide = 4,
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

	struct Logger
	{
	public:
		static void SetLogCallback(LogCallback cb, void* userData = nullptr)
		{
			callback = std::move(cb);
			logUserData = userData;
		}

		static auto GetLastError() -> ErrorCode
		{
			return lastError;
		}

		static void Log(LogMessage& message);

		// you're not really intended to use this
		// use the macros instead
		template <typename... Args>
		static void Log(
			LogLevel severity,
			std::format_string<Args...> message,
			std::string_view file,
			std::string_view function,
			int line,
			ErrorCode code,
			Args&&... args
		)
		{
			LogImpl(
				severity,
				message.get(),
				file,
				function,
				line,
				code,
				std::make_format_args(args...)
			);
		}

	private:
		static inline LogCallback callback{nullptr};
		static inline void* logUserData{nullptr};
		static inline ErrorCode lastError{ErrorCode::None};

		static void LogImpl(
			LogLevel severity,
			std::string_view message,
			std::string_view file,
			std::string_view function,
			int line,
			ErrorCode code,
			std::format_args args
		);
	};
}

#define mc_log(level, code, message, ...)                                      \
	mocca::Logger::Log(                                                        \
		level,                                                                 \
		message,                                                               \
		__FILE__,                                                              \
		__FUNCTION__,                                                          \
		__LINE__,                                                              \
		code,                                                                  \
		##__VA_ARGS__                                                          \
	)

#define mc_info(message, ...)                                                  \
	mc_log(                                                                    \
		mocca::LogLevel::Info,                                                 \
		mocca::ErrorCode::None,                                                \
		message,                                                               \
		##__VA_ARGS__                                                          \
	)

#define mc_warning(message, ...)                                               \
	mc_log(                                                                    \
		mocca::LogLevel::Warning,                                              \
		mocca::ErrorCode::None,                                                \
		message,                                                               \
		##__VA_ARGS__                                                          \
	)

#define mc_error(code, message, ...)                                           \
	mc_log(mocca::LogLevel::Error, code, message, ##__VA_ARGS__)

#ifdef DEBUG
#	define mc_debug(message, ...)                                             \
		mc_log(                                                                \
			mocca::LogLevel::Debug,                                            \
			mocca::ErrorCode::None,                                            \
			message,                                                           \
			##__VA_ARGS__                                                      \
		)
#else
#	define mc_debug(message, ...)
#endif

#define mc_assert(expr, message, ...)                                          \
	do                                                                         \
	{                                                                          \
		if (!(expr))                                                           \
		{                                                                      \
			mc_log(                                                            \
				mocca::LogLevel::Error,                                        \
				mocca::ErrorCode::AssertFailed,                                \
				"assertion \"{}\" failed: " message,                           \
				__FILE__,                                                      \
				__FUNCTION__,                                                  \
				__LINE__,                                                      \
				#expr,                                                         \
				##__VA_ARGS__                                                  \
			);                                                                 \
		}                                                                      \
	} while (0)
