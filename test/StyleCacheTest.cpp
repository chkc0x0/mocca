#include "TestSupport.h"
#include "core/Hooks.h"
#include "core/Node.h"
#include "style/ComputedStyle.h"
#include "style/StyleSet.h"
#include "yoga/YGNodeLayout.h"

namespace mtest
{
	using namespace mocca::styles;

	TEST(GroupBitsOfTest, MapsPropertiesToGroups)
	{
		EXPECT_EQ(
			GroupBitsOf[static_cast<std::size_t>(StyleProperty::BorderColor)],
			RenderGroupBit
		);
		EXPECT_EQ(
			GroupBitsOf[static_cast<std::size_t>(StyleProperty::Width)],
			GroupBit(StylePropertyGroup::LayoutSizing)
		);
		EXPECT_EQ(
			GroupBitsOf[static_cast<std::size_t>(StyleProperty::FontSize)],
			InheritedGroupBit
		);
	}

	TEST(ChangedGroupsTest, DefaultStylesEqual)
	{
		ComputedStyle a;
		ComputedStyle b;

		EXPECT_EQ(ComputedStyle::ChangedGroups(a, b), 0)
			<< "default styles (with NaN sizing sentinels) must compare equal";
	}

	TEST(ChangedGroupsTest, RenderChangeCategorised)
	{
		ComputedStyle a;
		ComputedStyle b;
		b.Set<StyleProperty::BackgroundColor>(mocca::colors::Red);

		EXPECT_EQ(ComputedStyle::ChangedGroups(a, b), RenderGroupBit);
	}

	TEST(ChangedGroupsTest, LayoutChangeCategorised)
	{
		ComputedStyle a;
		ComputedStyle b;
		b.Set<StyleProperty::Width>(SizingValue{mocca::Length{10.0F}});

		EXPECT_EQ(
			ComputedStyle::ChangedGroups(a, b),
			GroupBit(StylePropertyGroup::LayoutSizing)
		);
	}

	TEST(DependentGroupsTest, DefaultsToInherited)
	{
		StyleSet s;
		EXPECT_EQ(s.DependentGroups(), InheritedGroupBit);
	}

	TEST(DependentGroupsTest, TracksExplicitInherit)
	{
		StyleList list{
			detail::encodeStyleEntry(
				StyleProperty::BorderColor,
				StyleKeyword::Inherit
			)
		};

		StyleSet s;
		s.Assign(list);

		EXPECT_EQ(s.DependentGroups(), InheritedGroupBit | RenderGroupBit);
	}

	class StyleCacheTest : public SurfaceTest
	{
	public:
		explicit StyleCacheTest(const char* id = "mocca.test.stylecache")
			: SurfaceTest(id)
		{
		}
	};

	TEST_F(StyleCacheTest, RenderChangeNotDirtyYoga)
	{
		static std::optional<mocca::StateSetter<mocca::Color>> setColor;

		auto* s = AddSurface(
			{
				.Width = 100,
				.Height = 100,
				.Title = "render",
			},
			[]() -> mocca::Element
			{
				auto [color, set] = mocca::useState(mocca::colors::Red);
				if (!setColor)
				{
					setColor.emplace(set);
				}
				return mocca::box(
					mocca::BoxDescriptor{
						.Style =
							{
								Width(px(50)),
								Height(px(50)),
								BackgroundColor(color),
							},
					}
				);
			}
		);

		Tick();
		auto* root = s->Tree();
		ASSERT_NE(root, nullptr);
		ASSERT_FALSE(root->YogaDirty)
			<< "the first tick must have applied and cleared Yoga styles";

		(*setColor)(mocca::colors::Blue);
		Tick();

		EXPECT_EQ(root->ChangedGroups, RenderGroupBit);
		EXPECT_FALSE(root->YogaDirty);
		EXPECT_TRUE(rectHasColor(commandsOf(*s), mocca::colors::Blue));
	}

	TEST_F(StyleCacheTest, LayoutChangeDirtiesYoga)
	{
		static std::optional<mocca::StateSetter<float>> setWidth;

		auto* s = AddSurface(
			{
				.Width = 200,
				.Height = 100,
				.Title = "layout",
			},
			[]() -> mocca::Element
			{
				auto [w, set] = mocca::useState(50.0F);
				if (!setWidth)
				{
					setWidth.emplace(set);
				}
				return mocca::box(
					mocca::BoxDescriptor{
						.Style =
							{
								Width(px(w)),
								Height(px(50)),
								BackgroundColor(mocca::colors::Red),
							},
					}
				);
			}
		);

		Tick();
		auto* root = s->Tree();
		ASSERT_NE(root, nullptr);
		ASSERT_FALSE(root->YogaDirty);

		(*setWidth)(120.0F);
		Tick();

		EXPECT_NE(root->ChangedGroups & LayoutGroups, 0);
		EXPECT_FLOAT_EQ(YGNodeLayoutGetWidth(root->YogaNode), 120.0F);
	}

	TEST_F(StyleCacheTest, ChildFollowsExplicitInheritedBorder)
	{
		static std::optional<mocca::StateSetter<mocca::Color>> setColor;

		auto* s = AddSurface(
			{
				.Width = 200,
				.Height = 200,
				.Title = "inherit",
			},
			[]() -> mocca::Element
			{
				auto [color, set] = mocca::useState(mocca::colors::Red);
				if (!setColor)
				{
					setColor.emplace(set);
				}
				return mocca::box(
					mocca::BoxDescriptor{
						.Style =
							{
								Width(px(200)),
								Height(px(200)),
								BorderColor(color),
							},
						.Children =
							{
								mocca::box(
									mocca::BoxDescriptor{
										.Style =
											{
												Width(px(20)),
												Height(px(20)),
												detail::encodeStyleEntry(
													StyleProperty::BorderColor,
													StyleKeyword::Inherit
												),
											},
									}
								),
							},
					}
				);
			}
		);

		Tick();
		auto* root = s->Tree();
		ASSERT_NE(root, nullptr);
		ASSERT_EQ(root->Children.size(), 1U);
		auto* child = root->Children.front().get();
		ASSERT_NE(child, nullptr);
		EXPECT_EQ(
			child->Style.Get<StyleProperty::BorderColor>(),
			mocca::colors::Red
		);

		(*setColor)(mocca::colors::Blue);
		Tick();

		EXPECT_EQ(
			child->Style.Get<StyleProperty::BorderColor>(),
			mocca::colors::Blue
		);
	}
}
