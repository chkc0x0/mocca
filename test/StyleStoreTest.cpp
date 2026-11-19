#include "gtest/gtest.h"
#include "core/Application.h"
#include "style/StyleStore.h"
#include <limits>

namespace mtest
{
	using namespace mocca::styles;

	auto widthOf(const ComputedStyle& style) -> float
	{
		return std::get<Length>(style.Get<StyleProperty::Width>()).Value;
	}

	auto sizedStyle(float width) -> ComputedStyle
	{
		ComputedStyle style;
		style.Set<StyleProperty::Width>(SizingValue{Length{width}});
		return style;
	}

	TEST(StyleStoreTest, InternDedupes)
	{
		mocca::Application app("mocca.store.intern");
		auto& store = mocca::styles::detail::styleStore();

		ComputedStyle a = sizedStyle(10.0F);
		ComputedStyle b = sizedStyle(10.0F);

		StyleId id = store.Intern(a);
		EXPECT_NE(id, DefaultStyleId);
		EXPECT_EQ(store.Intern(b), id);
		EXPECT_EQ(store.Intern(ComputedStyle{}), DefaultStyleId);
	}

	TEST(StyleStoreTest, UniqueSlotCopyOnWrite)
	{
		mocca::Application app("mocca.store.cow");
		auto& store = mocca::styles::detail::styleStore();

		StyleId shared = store.Intern(sizedStyle(10.0F));
		std::size_t interned = store.InternedCount();

		StyleId unique = store.MakeUnique(store.Resolve(shared));
		EXPECT_TRUE(store.IsUnique(unique));
		EXPECT_NE(unique, shared);
		EXPECT_EQ(store.InternedCount(), interned);
		EXPECT_EQ(store.UniqueCount(), 1U);

		store.Mut(unique).Set<StyleProperty::Width>(SizingValue{Length{20.0F}});

		EXPECT_FLOAT_EQ(widthOf(store.Resolve(shared)), 10.0F);
		EXPECT_FLOAT_EQ(widthOf(store.Resolve(unique)), 20.0F);

		StyleId settled = store.Settle(unique);
		EXPECT_FALSE(store.IsUnique(settled));
		EXPECT_FLOAT_EQ(widthOf(store.Resolve(settled)), 20.0F);
		EXPECT_EQ(store.UniqueCount(), 0U);
		EXPECT_EQ(store.InternedCount(), interned + 1);
	}

	TEST(StyleStoreTest, SettleReusesInternedStyle)
	{
		mocca::Application app("mocca.store.settle");
		auto& store = mocca::styles::detail::styleStore();

		StyleId shared = store.Intern(sizedStyle(10.0F));
		StyleId unique = store.MakeUnique(store.Resolve(shared));
		std::size_t interned = store.InternedCount();

		EXPECT_EQ(store.Settle(unique), shared);
		EXPECT_EQ(store.InternedCount(), interned);
		EXPECT_EQ(store.UniqueCount(), 0U);
	}

	TEST(StyleStoreTest, HashNanZeroEquality)
	{
		mocca::Application app("mocca.store.hash");
		auto& store = mocca::styles::detail::styleStore();

		{
			ComputedStyle a = sizedStyle(1.0F);
			ComputedStyle b = sizedStyle(1.0F);
			a.Set<StyleProperty::MinWidth>(
				SizingValue{Length{std::numeric_limits<float>::quiet_NaN()}}
			);
			b.Set<StyleProperty::MinWidth>(
				SizingValue{Length{-std::numeric_limits<float>::quiet_NaN()}}
			);
			EXPECT_EQ(store.Intern(a), store.Intern(b));
		}

		{
			ComputedStyle a = sizedStyle(0.0F);
			ComputedStyle b = sizedStyle(-0.0F);
			EXPECT_EQ(store.Intern(a), store.Intern(b));
		}
	}
}
