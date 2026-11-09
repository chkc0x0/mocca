#include "a11y/A11y.h"
#include "core/Application.h"
#include "utils/TypeInterners.h"
#include <utility>

namespace mocca::aria
{
	template <typename T> auto hashValue(const T& value) -> size_t
	{
		return std::hash<T>{}(value);
	}

	template <typename T>
	auto hashValue(const std::optional<T>& value) -> size_t
	{
		size_t seed = value.has_value() ? 1 : 0;
		if (value.has_value())
		{
			detail::hashCombine(seed, hashValue(*value));
		}
		return seed;
	}

	template <typename T> auto hashValue(const std::vector<T>& value) -> size_t
	{
		size_t seed = value.size();
		for (const auto& element : value)
		{
			detail::hashCombine(seed, hashValue(element));
		}
		return seed;
	}

	auto PropsHash::operator()(const Props& props) const noexcept -> size_t
	{
		size_t seed = 0;
		auto hash = [&seed](const auto& value) -> void
		{ detail::hashCombine(seed, hashValue(value)); };

		hash(props.Role);
		hash(props.Id);

		hash(props.Name);
		hash(props.Description);
		hash(props.Value);
		hash(props.Placeholder);
		hash(props.RoleDescription);
		hash(props.KeyboardShortcut);
		hash(props.Language);

		hash(props.Disabled);
		hash(props.Hidden);
		hash(props.ReadOnly);
		hash(props.Required);
		hash(props.Busy);
		hash(props.Modal);
		hash(props.Multiselectable);
		hash(props.LiveAtomic);
		hash(props.Selected);
		hash(props.Expanded);
		hash(props.Toggled);
		hash(props.Live);
		hash(props.Orientation);
		hash(props.Invalid);
		hash(props.Current);
		hash(props.HasPopup);
		hash(props.AutoComplete);
		hash(props.SortDirection);

		hash(props.NumericValue);
		hash(props.MinValue);
		hash(props.MaxValue);
		hash(props.ValueStep);
		hash(props.ValueJump);
		hash(props.Level);
		hash(props.PositionInSet);
		hash(props.SizeOfSet);

		hash(props.LabelledBy);
		hash(props.DescribedBy);
		hash(props.Controls);
		hash(props.Details);
		hash(props.Owns);
		hash(props.FlowTo);

		return seed;
	}
}

namespace mocca
{
	auto a11y(aria::Props props) -> A11y
	{
		if (Application::main != nullptr)
		{
			return Application::main->GetInterners()->A11y.Intern(
				std::move(props)
			);
		}

		return std::make_shared<const aria::Props>(std::move(props));
	}

	auto a11y(aria::Role role, std::string name) -> A11y
	{
		aria::Props props;
		props.Role = role;
		props.Name = std::move(name);
		return a11y(std::move(props));
	}
}

namespace mocca::detail
{
	auto A11yInterner::Intern(aria::Props props) -> A11y
	{
		size_t hash = aria::PropsHash{}(props);
		auto& bucket = _buckets[hash];

		for (auto it = bucket.begin(); it != bucket.end();)
		{
			auto existing = it->Props.lock();
			if (existing == nullptr)
			{
				it = bucket.erase(it);
				continue;
			}

			if (*existing == props)
			{
				return existing;
			}

			++it;
		}

		auto interned = std::make_shared<const aria::Props>(std::move(props));
		bucket.push_back({.Props = interned});
		return interned;
	}

	void A11yInterner::Prune()
	{
		std::erase_if(
			_buckets,
			[](auto& bucket) -> bool
			{
				std::erase_if(
					bucket.second,
					[](const Entry& entry) -> bool
					{ return entry.Props.expired(); }
				);
				return bucket.second.empty();
			}
		);
	}
}
