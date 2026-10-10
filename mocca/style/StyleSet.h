#pragma once
#include "StyleList.h"
#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>
#include <vector>

namespace mocca::styles
{
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
