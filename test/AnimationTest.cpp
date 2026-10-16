#include "TestSupport.h"
#include "anim/Curve.h"
#include "anim/Interpolate.h"
#include "anim/Spring.h"
#include "core/Hooks.h"
#include <optional>

namespace mtest
{
	using AnimationTest = SurfaceTest;

	TEST(AnimationMath, BezierEndpoints)
	{
		EXPECT_FLOAT_EQ(mocca::anim::sample(mocca::anim::EaseInOut, 0.0F), 0.0F);
		EXPECT_FLOAT_EQ(mocca::anim::sample(mocca::anim::EaseInOut, 1.0F), 1.0F);
	}

	TEST(AnimationMath, SpringSettles)
	{
		mocca::anim::SpringState state;
		auto curve = mocca::anim::spring(200.0F, 20.0F);
		float value = 0.0F;
		for (int i = 0; i < 400; ++i)
		{
			value = mocca::anim::springStep(
				value,
				1.0F,
				state,
				curve,
				1.0F / 60.0F
			);
		}
		EXPECT_NEAR(value, 1.0F, 1e-2F);
		EXPECT_TRUE(mocca::anim::springSettled(value, 1.0F, state));
	}

	TEST(AnimationMath, InterpolatesTypes)
	{
		EXPECT_FLOAT_EQ(mocca::anim::lerp(0.0F, 10.0F, 0.5F), 5.0F);
		EXPECT_EQ(
			mocca::anim::interpolate(mocca::colors::Black, mocca::colors::Black, 0.5F),
			mocca::colors::Black
		);
		EXPECT_EQ(
			mocca::anim::interpolate(mocca::Length{0}, mocca::Length{10}, 1.0F),
			(mocca::Length{10})
		);
	}

	TEST(AnimationMath, ColorClampsOvershoot)
	{
		const auto One = mocca::Color::MixOklab(
			mocca::colors::Transparent,
			mocca::colors::Red,
			1.0
		);
		const auto Zero = mocca::Color::MixOklab(
			mocca::colors::Transparent,
			mocca::colors::Red,
			0.0
		);

		EXPECT_EQ(
			mocca::anim::interpolate(
				mocca::colors::Transparent,
				mocca::colors::Red,
				1.5F
			),
			One
		);
		EXPECT_EQ(
			mocca::anim::interpolate(
				mocca::colors::Transparent,
				mocca::colors::Red,
				-0.5F
			),
			Zero
		);
		EXPECT_EQ(One.A, 255);
	}

	TEST(AnimationMath, LengthClamps)
	{
		EXPECT_EQ(
			mocca::anim::interpolate(mocca::Length{10}, mocca::Length{20}, -2.0F),
			(mocca::Length{0})
		);
	}

	namespace
	{
		std::optional<mocca::StateSetter<bool>> animOn;
		float animLast = -1.0F;
		int animRenders = 0;

		auto AnimComponent(mocca::anim::Curve curve, float duration)
			-> mocca::Element
		{
			auto [on, setOn] = mocca::useState(false);
			if (!animOn)
			{
				animOn.emplace(setOn);
			}

			auto [value, setValue] =
				mocca::useAnimation(on ? 1.0F : 0.0F, curve, duration);
			(void)setValue;
			animLast = value;
			animRenders++;
			return box(50, 50);
		}
	}

	TEST_F(AnimationTest, HookAnimates)
	{
		animOn.reset();
		animLast = -1.0F;

		auto* s = AddSurface(
			{.Width = 50, .Height = 50, .Title = "hook"},
			[]() -> mocca::Element
			{ return AnimComponent(mocca::anim::EaseInOut, 0.16F); }
		);
		(void)s;

		Tick();
		EXPECT_FLOAT_EQ(animLast, 0.0F);

		(*animOn)(true);
		Tick(0.016, 3);
		EXPECT_GT(animLast, 0.0F);
		EXPECT_LT(animLast, 1.0F);

		Tick(0.016, 40);
		EXPECT_FLOAT_EQ(animLast, 1.0F);
	}

	TEST_F(AnimationTest, StopsRenderingWhenSettled)
	{
		animOn.reset();
		animRenders = 0;

		auto* s = AddSurface(
			{.Width = 50, .Height = 50, .Title = "settle"},
			[]() -> mocca::Element
			{ return AnimComponent(mocca::anim::EaseInOut, 0.1F); }
		);
		(void)s;

		Tick();
		(*animOn)(true);
		Tick(0.016, 60);

		int settled = animRenders;
		Tick(0.016, 30);

		EXPECT_EQ(animRenders, settled)
			<< "a settled animation must not keep the surface dirty";
		EXPECT_FALSE(Saw("effects have set state"))
			<< "a settled animation must not trigger the effect guard";
	}

	TEST_F(AnimationTest, SpringNotWarning)
	{
		animOn.reset();
		animLast = -1.0F;

		auto* s = AddSurface(
			{.Width = 50, .Height = 50, .Title = "spring"},
			[]() -> mocca::Element
			{
				return AnimComponent(mocca::anim::spring(200.0F, 18.0F), 0.2F);
			}
		);
		(void)s;

		Tick();
		(*animOn)(true);
		Tick(0.016, 60);

		EXPECT_NEAR(animLast, 1.0F, 1e-2F);
		EXPECT_FALSE(Saw("effects have set state"))
			<< "running animation should not warn";
	}

	namespace
	{
		std::optional<mocca::StateSetter<bool>> animToggler;

		auto TransitionComponent(bool withTransition) -> mocca::Element
		{
			using namespace mocca::styles;
			auto [on, setOn] = mocca::useState(false);
			if (!animToggler)
			{
				animToggler.emplace(setOn);
			}

			StyleList style{
				Width(px(40)),
				Height(px(40)),
				BackgroundColor(
					on ? mocca::colors::Red : mocca::colors::Transparent
				),
			};
			if (withTransition)
			{
				style.Add(
					Transition(
						mocca::styles::StyleProperty::BackgroundColor,
						mocca::styles::ms(100),
						mocca::anim::LinearCurve
					)
				);
			}
			return mocca::box(
				mocca::BoxDescriptor{.Style = std::move(style)}
			);
		}
	}

	TEST_F(AnimationTest, StyleTransitionEasesColor)
	{
		animToggler.reset();

		auto* s = AddSurface(
			{.Width = 40, .Height = 40, .Title = "transition"},
			[]() -> mocca::Element { return TransitionComponent(true); }
		);

		Tick();
		EXPECT_EQ(firstRect(commandsOf(*s))->Color, mocca::colors::Transparent);

		(*animToggler)(true);
		Tick(0.05, 1);
		(*animToggler)(true);

		auto mid = firstRect(commandsOf(*s))->Color;
		EXPECT_NE(mid, mocca::colors::Transparent);
		EXPECT_NE(mid, mocca::colors::Red);

		Tick(0.05, 4);
		EXPECT_EQ(firstRect(commandsOf(*s))->Color, mocca::colors::Red);
	}

	TEST_F(AnimationTest, SpringColorSettles)
	{
		animToggler.reset();

		auto* s = AddSurface(
			{.Width = 40, .Height = 40, .Title = "springcolor"},
			[]() -> mocca::Element
			{
				using namespace mocca::styles;
				auto [on, setOn] = mocca::useState(false);
				if (!animToggler)
				{
					animToggler.emplace(setOn);
				}

				return mocca::box(
					mocca::BoxDescriptor{
						.Style =
							{
								Width(px(40)),
								Height(px(40)),
								BackgroundColor(
									on ? mocca::colors::Red
									   : mocca::colors::Transparent
								),
								Transition(
									StyleProperty::BackgroundColor,
									mocca::anim::spring(200.0F, 18.0F)
								),
							},
					}
				);
			}
		);

		Tick();
		(*animToggler)(true);
		Tick(0.016, 160);

		EXPECT_EQ(firstRect(commandsOf(*s))->Color, mocca::colors::Red);
	}

	TEST_F(AnimationTest, NoAnimColorSnaps)
	{
		animToggler.reset();

		auto* s = AddSurface(
			{.Width = 40, .Height = 40, .Title = "snap"},
			[]() -> mocca::Element { return TransitionComponent(false); }
		);

		Tick();
		(*animToggler)(true);
		Tick();

		EXPECT_EQ(firstRect(commandsOf(*s))->Color, mocca::colors::Red);
	}
}
