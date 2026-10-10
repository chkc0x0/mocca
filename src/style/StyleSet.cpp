#include "style/StyleSet.h"
#include <algorithm>
#include <cstring>

namespace mocca::styles
{
	auto StyleSet::Assign(const StyleList& list) -> bool
	{
		const auto& src = list.Entries();
		if (src.size() == _entries.size())
		{
			if (src.empty() || memcmp(
								   _entries.data(),
								   src.data(),
								   src.size() * sizeof(StyleEntry)
							   ) == 0)
			{
				return false;
			}
			memcpy(
				static_cast<void*>(_entries.data()),
				src.data(),
				src.size() * sizeof(StyleEntry)
			);
			return true;
		}
		_entries = std::vector<StyleEntry>(src.begin(), src.end());
		return true;
	}

	auto StyleSet::Find(StyleProperty p) const -> const StyleEntry*
	{
		auto it = std::lower_bound(
			_entries.begin(),
			_entries.end(),
			p,
			[](const StyleEntry& e, StyleProperty k) -> bool
			{ return e.Prop < k; }
		);
		return (it != _entries.end() && (*it).Prop == p) ? &*it : nullptr;
	}

	void StyleSet::DiffStyles(
		std::span<const StyleEntry> before,
		std::span<const StyleEntry> after,
		StyleChangeFn&& onChange
	)
	{
		size_t i = 0;
		size_t j = 0;
		while (i < before.size() || j < after.size())
		{
			if (j == after.size() ||
				(i < before.size() && before[i].Prop < after[j].Prop))
			{
				onChange(
					StyleChange::Removed,
					&before[i],
					static_cast<const StyleEntry*>(nullptr)
				);
				++i;
			}
			else if (i == before.size() || after[j].Prop < before[i].Prop)
			{
				onChange(
					StyleChange::Added,
					static_cast<const StyleEntry*>(nullptr),
					&after[j]
				);
				++j;
			}
			else
			{
				if (!entryEqual(before[i], after[j]))
				{
					onChange(StyleChange::Changed, &before[i], &after[j]);
				}
				++i;
				++j;
			}
		}
	}
}
