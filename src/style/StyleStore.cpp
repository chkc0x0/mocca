#include "style/StyleStore.h"
#include "core/Application.h"
#include "core/Identifiers.h"
#include "core/Logger.h"
#include "utils/TypeInterners.h"
#include <cmath>
#include <functional>
#include <type_traits>
#include <variant>

namespace mocca::styles
{
	auto hashFloat(float value) -> std::size_t
	{
		if (std::isnan(value))
		{
			return 0x7fc00000U;
		}

		if (value == 0.0F)
		{
			value = 0.0F;
		}

		return std::hash<float>{}(value);
	}

	template <typename T>
		requires std::is_integral_v<T>
	auto hashOne(T value) -> std::size_t
	{
		return std::hash<T>{}(value);
	}

	template <typename T>
		requires std::is_enum_v<T>
	auto hashOne(T value) -> std::size_t
	{
		using U = std::underlying_type_t<T>;
		return hashOne(static_cast<U>(value));
	}

	auto hashOne(float value) -> std::size_t
	{
		return hashFloat(value);
	}

	auto hashOne(const Length& value) -> std::size_t
	{
		std::size_t h = hashOne(static_cast<std::uint8_t>(value.Unit));
		mocca::detail::hashCombine(h, hashFloat(value.Value));
		return h;
	}

	auto hashOne(const Color& value) -> std::size_t
	{
		return hashOne(value.ToRgba());
	}

	auto hashOne(const Vector2& value) -> std::size_t
	{
		std::size_t h = hashFloat(value.X);
		mocca::detail::hashCombine(h, hashFloat(value.Y));
		return h;
	}

	auto hashOne(const Vector2Length& value) -> std::size_t
	{
		std::size_t h = hashOne(value.X);
		mocca::detail::hashCombine(h, hashOne(value.Y));
		return h;
	}

	auto hashOne(const FontStyleType& value) -> std::size_t
	{
		std::size_t h = hashOne(value.Style);
		mocca::detail::hashCombine(h, hashFloat(value.Angle));
		return h;
	}

	template <typename... Ts>
	auto hashOne(const std::variant<Ts...>& value) -> std::size_t
	{
		std::size_t h = value.index();
		std::visit(
			[&](const auto& arm) -> void
			{ mocca::detail::hashCombine(h, hashOne(arm)); },
			value
		);
		return h;
	}

	auto hashSizing(const ComputedStyle::LayoutSizingProps& g) -> std::size_t
	{
		std::size_t h = 0;
#undef mc_styleProperty
#define mc_styleProperty(name, ...)                                            \
	mocca::detail::hashCombine(h, hashOne(g.name));
		mc_layoutSizingProps
#undef mc_styleProperty
			return h;
	}

	auto hashSpacing(const ComputedStyle::LayoutSpacingProps& g) -> std::size_t
	{
		std::size_t h = 0;
#undef mc_styleProperty
#define mc_styleProperty(name, ...)                                            \
	mocca::detail::hashCombine(h, hashOne(g.name));
		mc_layoutSpacingProps
#undef mc_styleProperty
			return h;
	}

	auto hashFlex(const ComputedStyle::LayoutFlexProps& g) -> std::size_t
	{
		std::size_t h = 0;
#undef mc_styleProperty
#define mc_styleProperty(name, ...)                                            \
	mocca::detail::hashCombine(h, hashOne(g.name));
		mc_layoutFlexProps
#undef mc_styleProperty
			return h;
	}

	auto hashPosition(const ComputedStyle::LayoutPositionProps& g)
		-> std::size_t
	{
		std::size_t h = 0;
#undef mc_styleProperty
#define mc_styleProperty(name, ...)                                            \
	mocca::detail::hashCombine(h, hashOne(g.name));
		mc_layoutPosProps
#undef mc_styleProperty
			return h;
	}

	auto hashRender(const ComputedStyle::RenderProps& g) -> std::size_t
	{
		std::size_t h = 0;
#undef mc_styleProperty
#define mc_styleProperty(name, ...)                                            \
	mocca::detail::hashCombine(h, hashOne(g.name));
		mc_renderPropertiesReset
#undef mc_styleProperty
			return h;
	}

	auto hashTransform(const ComputedStyle::TransformProps& g) -> std::size_t
	{
		std::size_t h = 0;
#undef mc_styleProperty
#define mc_styleProperty(name, ...)                                            \
	mocca::detail::hashCombine(h, hashOne(g.name));
		mc_transformProperties
#undef mc_styleProperty
			return h;
	}

	auto hashInherited(const ComputedStyle::InheritedProps& g) -> std::size_t
	{
		std::size_t h = 0;
#undef mc_styleProperty
#define mc_styleProperty(name, ...)                                            \
	mocca::detail::hashCombine(h, hashOne(g.name));
		mc_inheritProperties
#undef mc_styleProperty
			return h;
	}

	auto hashStyle(const ComputedStyle& style) -> std::size_t
	{
		std::size_t h = 0;
		mocca::detail::hashCombine(h, hashSizing(style.LayoutSizing));
		mocca::detail::hashCombine(h, hashSpacing(style.LayoutSpacing));
		mocca::detail::hashCombine(h, hashFlex(style.LayoutFlex));
		mocca::detail::hashCombine(h, hashPosition(style.LayoutPosition));
		mocca::detail::hashCombine(h, hashRender(style.Render));
		mocca::detail::hashCombine(h, hashTransform(style.Transform));
		mocca::detail::hashCombine(h, hashInherited(style.Inherited));
		return h;
	}

	StyleStore::StyleStore()
	{
		_rehash(64);
	}

	auto getDefaultStyle() -> const ComputedStyle&
	{
		static const ComputedStyle Style;
		return Style;
	}

	auto StyleStore::Intern(const ComputedStyle& value) -> StyleId
	{
		if (value == getDefaultStyle())
		{
			return DefaultStyleId;
		}

		if (_lastHit != 0 && _values[_lastHit - 1] == value)
		{
			return _lastHit;
		}

		std::size_t hash = hashStyle(value);

		if (StyleId found = _find(value, hash); found != 0)
		{
			_lastHit = found;
			return found;
		}

		if ((_interned + 1) * 10 > _buckets.size() * 7)
		{
			_rehash(_buckets.size() * 2);
		}

		StyleId id = _allocate();
		_values[id - 1] = value;
		_slots[id - 1].State = SlotKind::Interned;
		_insert(id, hash);
		++_interned;
		_lastHit = id;
		return id;
	}

	auto StyleStore::MakeUnique(const ComputedStyle& value) -> StyleId
	{
		StyleId id = _allocate();
		_values[id - 1] = value;
		_slots[id - 1].State = SlotKind::Unique;
		++_unique;
		return id;
	}

	auto StyleStore::Mut(StyleId id) -> ComputedStyle&
	{
		mc_assert(
			id != 0 && id <= _values.size() &&
				_slots[id - 1].State == SlotKind::Unique,
			"mutating a shared computed style"
		);
		return _values[id - 1];
	}

	auto StyleStore::Settle(StyleId id) -> StyleId
	{
		if (id == 0 || _slots[id - 1].State != SlotKind::Unique)
		{
			return id;
		}

		StyleId interned = Intern(_values[id - 1]);
		Release(id);
		return interned;
	}

	void StyleStore::Release(StyleId id)
	{
		if (id == 0 || id > _values.size())
		{
			return;
		}

		Slot& slot = _slots[id - 1];
		if (slot.State != SlotKind::Unique)
		{
			return;
		}

		slot.State = SlotKind::Free;
		slot.NextFree = _freeHead;
		_freeHead = id;
		--_unique;
	}

	auto StyleStore::Resolve(StyleId id) const -> const ComputedStyle&
	{
		if (id == 0)
		{
			return getDefaultStyle();
		}

		mc_assert(id <= _values.size(), "invalid style id");
		return _values[id - 1];
	}

	auto StyleStore::IsUnique(StyleId id) const -> bool
	{
		return id != 0 && id <= _values.size() &&
			   _slots[id - 1].State == SlotKind::Unique;
	}

	auto StyleStore::_allocate() -> StyleId
	{
		if (_freeHead != 0)
		{
			StyleId id = _freeHead;
			_freeHead = _slots[id - 1].NextFree;
			_slots[id - 1] = Slot{};
			return id;
		}

		_values.emplace_back();
		_slots.emplace_back();
		auto id = static_cast<StyleId>(_values.size());
		mc_assert(
			static_cast<std::size_t>(id) == _values.size(),
			"style store overflow"
		);
		return id;
	}

	auto StyleStore::_find(const ComputedStyle& value, std::size_t hash) const
		-> StyleId
	{
		if (_buckets.empty())
		{
			return 0;
		}

		std::size_t mask = _buckets.size() - 1;
		std::size_t i = hash & mask;

		while (true)
		{
			StyleId id = _buckets[i];
			if (id == 0)
			{
				return 0;
			}

			if (_values[id - 1] == value)
			{
				return id;
			}

			i = (i + 1) & mask;
		}
	}

	void StyleStore::_insert(StyleId id, std::size_t hash)
	{
		std::size_t mask = _buckets.size() - 1;
		std::size_t i = hash & mask;

		while (_buckets[i] != 0)
		{
			i = (i + 1) & mask;
		}

		_buckets[i] = id;
	}

	void StyleStore::_rehash(std::size_t capacity)
	{
		std::size_t size = 64;
		while (size < capacity)
		{
			size <<= 1;
		}

		_buckets.assign(size, 0);

		std::size_t mask = size - 1;
		for (std::size_t i = 0; i < _slots.size(); ++i)
		{
			if (_slots[i].State != SlotKind::Interned)
			{
				continue;
			}

			auto id = static_cast<StyleId>(i + 1);
			std::size_t hash = hashStyle(_values[i]);
			std::size_t j = hash & mask;

			while (_buckets[j] != 0)
			{
				j = (j + 1) & mask;
			}

			_buckets[j] = id;
		}
	}
}

namespace mocca::styles::detail
{
	auto styleStore() -> StyleStore&
	{
		mc_assert(
			Application::main != nullptr,
			"no application for style store"
		);
		return Application::main->GetInterners()->Styles;
	}
}
