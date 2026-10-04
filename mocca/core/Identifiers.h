#pragma once
#include <cstdint>
#include <string_view>
#include <string>

namespace mocca
{
	constexpr auto hashString(std::string_view str) -> uint64_t
	{
		uint64_t hash = 14695981039346656037ULL;
		for (char c : str)
		{
			hash ^= static_cast<uint8_t>(c);
			hash *= 1099511628211ULL;
		}
		return hash;
	}
	
	struct ApplicationID
	{
	public:
		ApplicationID(const std::string& id);
		ApplicationID() : _id("<not set>") {};
		ApplicationID(const ApplicationID&) = delete;
		ApplicationID(ApplicationID&&) noexcept = default;
		auto operator=(const ApplicationID&) -> ApplicationID& = delete;
		auto operator=(ApplicationID&&) noexcept -> ApplicationID& = default;

		static auto ValidateID(std::string_view id) -> bool;
		[[nodiscard]] auto GetCompoundID() const -> std::string;

		[[nodiscard]] auto Name() const -> std::string_view;
		[[nodiscard]] auto Organization() const -> std::string_view;
		[[nodiscard]] auto Domain() const -> std::string_view;

	private:
		std::string _id;
		std::size_t _firstDot = std::string_view::npos;
		std::size_t _lastDot = std::string_view::npos;
	};

	struct ElementKey
	{
		ElementKey(std::string_view str) : _value(hashString(str)) {};
		ElementKey(uint64_t value) : _value(value) {};
		ElementKey(int value) : _value(static_cast<uint64_t>(value)) {};
		ElementKey() : _value(0) {};
		ElementKey(const char* str) : ElementKey(std::string_view(str)) {};

		operator uint64_t() const
		{
			return _value;
		}

		auto operator<=>(const ElementKey& other) const
		{
			return _value <=> other._value;
		}

		auto operator==(const ElementKey& other) const -> bool
		{
			return _value == other._value;
		}
		auto operator==(int other) const -> bool
		{
			return _value == static_cast<uint64_t>(other);
		}
		auto operator==(std::string_view other) const -> bool
		{
			return ElementKey(other) == *this;
		}

	private:
		uint64_t _value;
	};

	namespace detail
	{
		using NodeId = std::uint64_t;
	}
}