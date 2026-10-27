#include "TestSupport.h"
#include "core/Hooks.h"
#include "core/Node.h"
#include "core/NodeRef.h"
#include "style/StyleEnums.h"
#include <optional>

namespace mtest
{
	using namespace mocca::styles;

	using NodeRefTest = SurfaceTest;

	namespace
	{
		auto fillBox(float w, float h, mocca::NodeRef* ref = nullptr)
			-> mocca::Element
		{
			return mocca::box(
				mocca::BoxDescriptor{
					.Style =
						{
							Width(px(w)),
							Height(px(h)),
						},
					.Ref = ref,
				}
			);
		}
	}

	TEST_F(NodeRefTest, UseNodeResolvesFirstTick)
	{
		mocca::NodeRef* ref = nullptr;
		bool validDuringRender = true;
		int renders = 0;

		auto* s = AddSurface(
			{
				.Width = 100,
				.Height = 100,
				.Title = "noderef",
			},
			[&]() -> mocca::Element
			{
				auto* current = mocca::useNode();
				if (renders == 0)
				{
					ref = current;
					validDuringRender = current->Valid();
				}
				else
				{
					EXPECT_EQ(current, ref)
						<< "ref must be stable";
				}
				++renders;

				return fillBox(100, 100, current);
			}
		);

		ASSERT_NE(s, nullptr);
		EXPECT_EQ(ref, nullptr) << "nothing on first tick";

		Tick();

		ASSERT_NE(ref, nullptr);
		EXPECT_FALSE(validDuringRender)
			<< "ids are assigned after render";
		EXPECT_TRUE(ref->Valid());
		EXPECT_NE(ref->Id(), 0U);
		EXPECT_EQ(ref->Surface(), s);
		EXPECT_FLOAT_EQ(ref->Rect().Width, 100.0F);
		EXPECT_FLOAT_EQ(ref->Rect().Height, 100.0F);
	}

	TEST_F(NodeRefTest, RectParentScrollsLayoutNot)
	{
		mocca::NodeRef* target = nullptr;

		auto* s = AddSurface(
			{
				.Width = 100,
				.Height = 100,
				.Title = "scrolled",
			},
			[&]() -> mocca::Element
			{
				target = mocca::useNode();

				return mocca::box(
					mocca::BoxDescriptor{
						.Style =
							{
								Width(px(100)),
								Height(px(100)),
								Overflow(mocca::styles::OverflowType::Scroll),
							},
						.Children = {
							mocca::box(
								mocca::BoxDescriptor{
									.Style =
										{
											Width(px(200)),
											Height(px(200)),
										},
									.Children = {
										mocca::box(
											mocca::BoxDescriptor{
												.Style =
													{
														Width(px(20)),
														Height(px(20)),
														MarginLeft(px(10)),
														MarginTop(px(50)),
													},
												.Ref = target,
											}
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

		ASSERT_NE(target, nullptr);
		ASSERT_TRUE(target->Valid());

		auto layout = target->LayoutRect();
		EXPECT_FLOAT_EQ(layout.X, 10.0F);
		EXPECT_FLOAT_EQ(layout.Y, 50.0F);

		s->Tree()->ScrollOffset = {.X = 0, .Y = 20};

		auto painted = target->Rect();
		EXPECT_FLOAT_EQ(painted.X, 10.0F);
		EXPECT_FLOAT_EQ(painted.Y, 30.0F)
			<< "painted rect must subtract ancestor scroll offsets";
		EXPECT_FLOAT_EQ(painted.Width, 20.0F);
	}

	TEST_F(NodeRefTest, ScrollIntoView)
	{
		mocca::NodeRef* target = nullptr;

		auto* s = AddSurface(
			{
				.Width = 100,
				.Height = 100,
				.Title = "scrollintoview",
			},
			[&]() -> mocca::Element
			{
				target = mocca::useNode();

				return mocca::box(
					mocca::BoxDescriptor{
						.Style =
							{
								Width(px(100)),
								Height(px(100)),
								Overflow(mocca::styles::OverflowType::Scroll),
							},
						.Children = {
							mocca::box(
								mocca::BoxDescriptor{
									.Style =
										{
											Width(px(200)),
											Height(px(200)),
										},
									.Children = {
										mocca::box(
											mocca::BoxDescriptor{
												.Style =
													{
														Width(px(20)),
														Height(px(20)),
														MarginTop(px(150)),
													},
												.Ref = target,
											}
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

		ASSERT_NE(target, nullptr);
		ASSERT_TRUE(target->Valid());

		auto* root = s->Tree();
		ASSERT_NE(root, nullptr);

		target->ScrollIntoView();
		EXPECT_FLOAT_EQ(root->ScrollOffset.Y, 70.0F);
		EXPECT_FLOAT_EQ(target->Rect().Y, 80.0F);
		EXPECT_TRUE(s->IsDirty()) << "scrolling must dirty the surface";

		root->ScrollOffset = {.X = 0, .Y = 0};
		target->ScrollRectIntoView(
			{.X = 0, .Y = 500, .Width = 20, .Height = 20}
		);
		EXPECT_FLOAT_EQ(root->ScrollOffset.Y, 100.0F)
			<< "scroll offsets must clamp to the content extent";
	}

	TEST_F(NodeRefTest, ScrollIntoViewNestedScrollables)
	{
		mocca::NodeRef* target = nullptr;

		auto* s = AddSurface(
			{
				.Width = 100,
				.Height = 100,
				.Title = "nested",
			},
			[&]() -> mocca::Element
			{
				target = mocca::useNode();

				auto inner = mocca::box(
					mocca::BoxDescriptor{
						.Style =
							{
								Width(px(100)),
								Height(px(80)),
								Overflow(mocca::styles::OverflowType::Scroll),
							},
						.Children = {
							mocca::box(
								mocca::BoxDescriptor{
									.Style =
										{
											Width(px(100)),
											Height(px(200)),
										},
									.Children = {
										fillBox(100, 150),
										mocca::box(
											mocca::BoxDescriptor{
												.Style =
													{
														Width(px(20)),
														Height(px(20)),
													},
												.Ref = target,
											}
										),
									},
								}
							),
						},
					}
				);

				return mocca::box(
					mocca::BoxDescriptor{
						.Style =
							{
								Width(px(100)),
								Height(px(100)),
								Overflow(mocca::styles::OverflowType::Scroll),
							},
						.Children = {
							mocca::box(
								mocca::BoxDescriptor{
									.Style =
										{
											Width(px(100)),
											Height(px(400)),
										},
									.Children = {
										fillBox(100, 200),
										std::move(inner),
									},
								}
							),
						},
					}
				);
			}
		);

		Tick();

		ASSERT_NE(target, nullptr);
		ASSERT_TRUE(target->Valid());

		auto* outer = s->Tree();
		ASSERT_NE(outer, nullptr);
		ASSERT_EQ(outer->Children.size(), 1U);
		auto* outerContent = outer->Children[0].get();
		ASSERT_EQ(outerContent->Children.size(), 2U);
		auto* inner = outerContent->Children[1].get();

		target->ScrollIntoView();

		EXPECT_FLOAT_EQ(inner->ScrollOffset.Y, 90.0F);
		EXPECT_FLOAT_EQ(outer->ScrollOffset.Y, 180.0F)
			<< "the outermost scrollable is adjusted last";
		EXPECT_FLOAT_EQ(target->Rect().Y, 80.0F);
	}

	TEST_F(NodeRefTest, RefInvalidWhenNodeRemoved)
	{
		mocca::NodeRef* target = nullptr;
		std::optional<mocca::StateSetter<bool>> setShow;

		auto* s = AddSurface(
			{
				.Width = 100,
				.Height = 100,
				.Title = "removal",
			},
			[&]() -> mocca::Element
			{
				auto [show, set] = mocca::useState(true);
				setShow = set;

				target = mocca::useNode();

				std::vector<mocca::Element> children;
				if (show)
				{
					children.push_back(fillBox(20, 20, target));
				}

				return mocca::box(
					mocca::BoxDescriptor{
						.Style =
							{
								Width(px(100)),
								Height(px(100)),
							},
						.Children = std::move(children),
					}
				);
			}
		);

		(void)s;
		Tick();

		ASSERT_NE(target, nullptr);
		ASSERT_TRUE(target->Valid());
		ASSERT_TRUE(setShow.has_value());

		(*setShow)(false);
		Tick();

		EXPECT_FALSE(target->Valid());
		EXPECT_TRUE(target->Rect().IsEmpty());
	}
}
