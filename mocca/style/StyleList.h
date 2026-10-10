#pragma once
#include "StyleEntry.h"
#include <cstddef>
#include <tuple>
#include <vector>

namespace mocca::styles
{
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
}
