#pragma once
#include <deque>
#include <functional>
#include <cstdint>

namespace mocca::detail
{
	template <class T, class Hash = std::hash<T>, class Eq = std::equal_to<T>>
	class Interner
	{
	public:
		using Id = uint32_t;
		
		auto Intern(const T& v) -> uint32_t
		{
			if (auto it = _index.find(v); it != _index.end())
			{
				return it->second;
			}
			Id id = _values.size() + 1;
			_values.push_back(v);
			_index.emplace(_values.back(), id);
			return id;
		}

		auto Get(Id id) const -> const T&
		{
			return _values[id - 1];
		}

	private:
		std::deque<T> _values;
		std::unordered_map<std::reference_wrapper<const T>, Id, Hash, Eq>
			_index;
	};
}