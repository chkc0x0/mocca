#include "style/StyleList.h"
#include "style/StyleValue.h"
#include <algorithm>

namespace mocca::styles
{
	auto mergeTransitions(
		const StyleEntry& existing,
		const StyleEntry& incoming
	) -> StyleEntry
	{
		TransitionList merged = detail::resolveTransition(
			existing.Payload.Handle
		);
		const TransitionList& extra = detail::resolveTransition(
			incoming.Payload.Handle
		);

		for (const auto& spec : extra.Specs)
		{
			if (std::find(merged.Specs.begin(), merged.Specs.end(), spec) ==
				merged.Specs.end())
			{
				merged.Specs.push_back(spec);
			}
		}

		StyleEntry out;
		out.Prop = StyleProperty::Transition;
		out.Type = StyleValueType::Handle;
		out.Payload.Handle = detail::internTransition(merged);
		return out;
	}

	void StyleList::Add(const StyleEntry& e)
	{
		auto it = _entries.end();
		while (it != _entries.begin() && (*(it - 1)).Prop > e.Prop)
		{
			--it;
		}
		if (it != _entries.begin() && (*(it - 1)).Prop == e.Prop)
		{
			auto& previous = *(it - 1);
			if (e.Prop == StyleProperty::Transition &&
				e.Type == StyleValueType::Handle &&
				previous.Type == StyleValueType::Handle)
			{
				previous = mergeTransitions(previous, e);
			}
			else
			{
				previous = e;
			}
		}
		else
		{
			auto r = _entries.insert(it, e);

			if (e.Prop == StyleProperty::FontSize)
			{
				std::iter_swap(_entries.begin(), r);
			}
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
