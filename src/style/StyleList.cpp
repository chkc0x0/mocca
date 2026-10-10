#include "style/StyleList.h"
#include <algorithm>

namespace mocca::styles
{
	void StyleList::Add(const StyleEntry& e)
	{
		auto it = _entries.end();
		while (it != _entries.begin() && (*(it - 1)).Prop > e.Prop)
		{
			--it;
		}
		if (it != _entries.begin() && (*(it - 1)).Prop == e.Prop)
		{
			*(it - 1) = e;
		}
		else
		{
			_entries.insert(it, e);
		}
	}

	auto StyleList::Find(StyleProperty p) const -> const StyleEntry*
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
}
