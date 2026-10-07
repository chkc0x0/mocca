#pragma once
#include <cstdint>
#include <cstring>
#include <functional>
#include <span>
#include <vector>
#include "StyleEnums.h"
#include "Length.h"
#include "utils/Color.h"

namespace mocca::styles
{
	enum class StyleProperty : char
	{
		None,
#undef mc_styleProperty
#define mc_styleProperty(name, ...) name,
#include "styles.def"
		mc_layoutProperties mc_renderProperties
#undef mc_styleProperty
			StylePropertyCount,
	};

	enum class StyleValueType : char
	{
		Keyword,
		Value,
		AuxKeyword,
		Length,
		Color,
	};

	struct StyleEntry
	{
		StyleProperty Prop = StyleProperty::None;
		StyleValueType Type = StyleValueType::Keyword;
		uint8_t Aux = 0;
		uint8_t Pad = 0;
		union
		{
			StyleKeyword Keyword;
			float Float;
			int Int = 0;
			struct Length Length;
			struct Color Color;
		} Payload;
	};

	template <size_t N> struct StyleEntryGroup
	{
		std::array<StyleEntry, N> Entries;
	};

	namespace detail
	{
		template <class T>
		inline constexpr bool IsStyleEntry = std::same_as<T, StyleEntry>;
		template <std::size_t N>
		inline constexpr bool IsStyleEntry<StyleEntryGroup<N>> = true;
	}

	auto entryEqual(const StyleEntry& a, const StyleEntry& b) -> bool;

	class StyleList
	{
	public:
		StyleList() = default;

		template <class... Items>
			requires(
				sizeof...(Items) > 0 && (detail::IsStyleEntry<Items> && ...)
			)
		StyleList(const Items&... items)
		{
			_entries.reserve((ItemCount<Items>() + ...));
			(Add(items), ...);
		}

		void Add(const StyleEntry& e);
		
		template <std::size_t N> void Add(const StyleEntryGroup<N>& g)
		{
			for (const StyleEntry& e : g.Entries)
			{
				Add(e);
			}
		}
		
		template <class Item> void AddIf(bool cond, const Item& item)
		{
			if (cond)
			{
				Add(item);
			}
		}

		auto operator+=(const StyleList& rhs) -> StyleList&
		{
			for (const StyleEntry& e : rhs._entries)
			{
				Add(e);
			}
			return *this;
		}

		friend auto operator+(StyleList lhs, const StyleList& rhs) -> StyleList
		{
			lhs += rhs;
			return lhs;
		}

		[[nodiscard]] auto Find(StyleProperty p) const -> const StyleEntry*;
		
		void Clear()
		{
			_entries.clear();
		}
		[[nodiscard]] auto Size() const -> std::size_t
		{
			return _entries.size();
		}
		[[nodiscard]] auto Empty() const -> bool
		{
			return _entries.empty();
		}
		[[nodiscard]] auto Entries() const -> const std::vector<StyleEntry>&
		{
			return _entries;
		}
		[[nodiscard]] auto begin() const
		{
			return _entries.begin();
		}
		[[nodiscard]] auto end() const
		{
			return _entries.end();
		}

	private:
		template <class Item> static constexpr auto ItemCount() -> std::size_t
		{
			if constexpr (std::same_as<Item, StyleEntry>)
			{
				return 1;
			}
			else
			{
				return std::tuple_size_v<decltype(Item::Entries)>;
			}
		}

		std::vector<StyleEntry> _entries;
	};

	enum class StyleChange : std::uint8_t
	{
		Added,
		Removed,
		Changed,
	};

	class StyleSet
	{
	public:
		StyleSet() = default;
		explicit StyleSet(const StyleList& list)
		{
			Assign(list);
		}

		auto Assign(const StyleList& list) -> bool;

		[[nodiscard]] auto Find(StyleProperty p) const -> const StyleEntry*;

		[[nodiscard]] auto Entries() const -> std::span<const StyleEntry>
		{
			return _entries;
		}
		[[nodiscard]] auto Size() const -> std::size_t
		{
			return _entries.size();
		}
		[[nodiscard]] auto Empty() const -> bool
		{
			return _entries.empty();
		}
		[[nodiscard]] auto begin() const
		{
			return _entries.begin();
		}
		[[nodiscard]] auto end() const
		{
			return _entries.end();
		}

		using StyleChangeFn = std::function<
			void(StyleChange, const StyleEntry*, const StyleEntry*)>;

		static void DiffStyles(
			std::span<const StyleEntry> before,
			std::span<const StyleEntry> after,
			StyleChangeFn&& onChange
		);

	private:
		std::vector<StyleEntry> _entries;
	};
}