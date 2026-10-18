#pragma once
#include "core/Identifiers.h"
#include <algorithm>
#include <compare>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <span>
#include <string>
#include <vector>

namespace mocca::styles
{
	struct FontFamilyType
	{
	public:
		std::vector<std::string> Families;

		constexpr auto operator==(const FontFamilyType&) const
			-> bool = default;
	};

	struct FontTag
	{
	public:
		constexpr FontTag() = default;

		constexpr FontTag(const char (&s)[5])
		{
			_data = static_cast<uint32_t>(s[0]) << 24 |
					static_cast<uint32_t>(s[1]) << 16 |
					static_cast<uint32_t>(s[2]) << 8 |
					static_cast<uint32_t>(s[3]);
		}

		constexpr auto operator==(const FontTag&) const -> bool = default;
		constexpr auto operator<=>(const FontTag&) const
			-> std::strong_ordering = default;

		constexpr operator uint32_t() const
		{
			return _data;
		}

	private:
		uint32_t _data{0};
	};

	template <typename T> struct FontTagList
	{
		struct Value
		{
			FontTag Tag;
			T Value;

			friend auto operator==(const struct Value&, const struct Value&)
				-> bool = default;
		};

		std::vector<Value> Axes;

		FontTagList() = default;

		FontTagList(std::span<const Value> in)
		{
			for (const Value& a : in)
			{
				auto it = std::ranges::find(Axes, a.Tag, &Value::Tag);
				if (it != Axes.end())
				{
					it->Value = a.Value;
				}
				else
				{
					Axes.push_back(a);
				}
			}
			std::ranges::sort(Axes, {}, &Value::Tag);
		}

		FontTagList(std::initializer_list<Value> in)
			: FontTagList(std::span<const Value>(in.begin(), in.end()))
		{
		}

		friend auto
		operator==(const FontTagList&, const FontTagList&) -> bool = default;
	};

	using FontVariationsType = FontTagList<float>;
	using FontFeaturesType = FontTagList<uint32_t>;
}

namespace std
{
	template <> struct hash<mocca::styles::FontFamilyType>
	{
		auto operator()(const mocca::styles::FontFamilyType& f) const noexcept
			-> size_t
		{
			size_t h = std::hash<size_t>{}(f.Families.size());
			for (const std::string& name : f.Families)
			{
				mocca::hashCombine(h, std::hash<std::string>{}(name));
			}
			return h;
		}
	};

	template <> struct hash<mocca::styles::FontTagList<float>>
	{
		auto operator()(const mocca::styles::FontTagList<float>& v) const noexcept
			-> size_t
		{
			size_t h = std::hash<size_t>{}(v.Axes.size());
			for (const auto& a : v.Axes)
			{
				mocca::hashCombine(h, std::hash<uint32_t>{}(a.Tag));
				mocca::hashCombine(h, std::hash<float>{}(a.Value));
			}
			return h;
		}
	};

	template <> struct hash<mocca::styles::FontTagList<uint32_t>>
	{
		auto operator()(const mocca::styles::FontTagList<uint32_t>& v)
			const noexcept -> size_t
		{
			size_t h = std::hash<size_t>{}(v.Axes.size());
			for (const auto& a : v.Axes)
			{
				mocca::hashCombine(h, std::hash<uint32_t>{}(a.Tag));
				mocca::hashCombine(h, std::hash<uint32_t>{}(a.Value));
			}
			return h;
		}
	};
}
