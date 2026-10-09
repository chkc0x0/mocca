#include "style/StyleSparse.h"

namespace mocca::styles
{
	auto entryEqual(const StyleEntry& a, const StyleEntry& b) -> bool
	{
		return memcmp(&a, &b, sizeof(StyleEntry)) == 0;
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

	void ComputedStyle::Apply(const StyleEntry& e, ComputedStyle& parent)
	{
#undef mc_styleProperty
#define mc_styleProperty(name, ...)                                            \
	case StyleProperty::name:                                                  \
		_ref<StyleProperty::name>() = e.Resolve(                               \
			parent._ref<StyleProperty::name>(),                                \
			PropTraits<StyleProperty::name>()                                  \
		);                                                                     \
		break;

		switch (e.Prop)
		{
			mc_layoutProperties mc_renderProperties default
				: mc_error(
					  ErrorCode::InvalidArgument,
					  "how did we get to this style?"
				  );
			break;
		}
	}

	auto ComputedStyle::Cascade(const StyleSet& s, ComputedStyle& parent)
		-> ComputedStyle
	{
		ComputedStyle out;
		out.Inherited = parent.Inherited;

		for (const auto& e : s.Entries())
		{
			out.Apply(e, parent);
		}

		return out;
	}
}