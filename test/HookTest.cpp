#include "TestSupport.h"

namespace mtest
{
	using HookTest = SurfaceTest;

	namespace
	{
		struct Counters
		{
			int Renders = 0;
			int Effects = 0;
			int Sets = 0;
			int LastValue = -1;
			std::vector<int> Order;
		};

		auto C() -> Counters&
		{
			static Counters c;
			return c;
		}

		auto Reset() -> void { C() = Counters{}; }
	}

	TEST_F(HookTest, StateSettlesOneFrame)
	{
		Reset();

		auto* s = AddSurface(
			{
				.Width = 100,
				.Height = 100,
				.Title = "state",
			},
			[]() -> mocca::Element
			{
				auto& c = C();
				c.Renders++;
				auto [v, setV] = mocca::useState(0);
				c.LastValue = v;
				if (v < 3)
				{
					c.Sets++;
					setV(v + 1);
				}
				return box(50, 50);
			}
		);

		Tick();

		EXPECT_EQ(C().Sets, 3);
		EXPECT_EQ(C().LastValue, 3)
			<< "the settle loop must run the state through to convergence in "
			   "one tick";
		EXPECT_GE(C().Renders, 4)
			<< "each state change must drive a re-render";

		Tick();
		EXPECT_EQ(C().LastValue, 3) << "state must survive a later render";
		(void)s;
	}

	TEST_F(HookTest, LayoutEffect)
	{
		Reset();

		(void)AddSurface(
			{
				.Width = 100,
				.Height = 100,
				.Title = "settle",
			},
			[]() -> mocca::Element
			{
				auto& c = C();
				c.Renders++;
				auto [v, setV] = mocca::useState(0);
				mocca::useLayoutEffect(
					[setV, &v]()
				 -> void	{
						if (v < 5)
						{
							C().Sets++;
							setV(v + 1);
						}
					}
				);
				return box(50, 50);
			}
		);

		Tick();

		EXPECT_EQ(C().Sets, 5);
		EXPECT_EQ(C().Renders, 6)
			<< "a converging effect must exit the settle loop after 6 passes";
	}

	TEST_F(HookTest, RunawayLayoutEffectCapped)
	{
		Reset();

		(void)AddSurface(
			{
				.Width = 100,
				.Height = 100,
				.Title = "runaway",
			},
			[]() -> mocca::Element
			{
				C().Renders++;
				auto [v, setV] = mocca::useState(0);
				mocca::useLayoutEffect(
					[setV, &v]() -> void { setV(v + 1); }
				);
				return box(50, 50);
			}
		);

		Tick();

		EXPECT_EQ(C().Renders, 16)
			<< "the runaway cap must fire";
	}

	TEST_F(HookTest, RunawayEffectCapped)
	{
		Reset();

		(void)AddSurface(
			{
				.Width = 100,
				.Height = 100,
				.Title = "runaway",
			},
			[]() -> mocca::Element
			{
				C().Renders++;
				auto [v, setV] = mocca::useState(0);
				mocca::useEffect([setV, &v]() -> void { setV(v + 1); });
				return box(50, 50);
			}
		);

		Tick();

		EXPECT_EQ(C().Renders, 16) << "the runaway cap must fire";
	}

	TEST_F(HookTest, LayoutEffectOrder)
	{
		Reset();

		(void)AddSurface(
			{
				.Width = 100,
				.Height = 100,
				.Title = "order",
			},
			[]() -> mocca::Element
			{
				auto& c = C();
				c.Order.push_back(1);
				mocca::useLayoutEffect([]()
				{ C().Order.push_back(2); });
				mocca::useEffect([]()
			 -> void	{ C().Order.push_back(3); });
				return box(50, 50);
			}
		);

		Tick();

		ASSERT_EQ(C().Order.size(), 3U)
			<< "expected render, layout effect, then effect";
		EXPECT_EQ(C().Order[0], 1);
		EXPECT_EQ(C().Order[1], 2) << "useLayoutEffect is pre-paint";
		EXPECT_EQ(C().Order[2], 3) << "useEffect is post-paint";
	}

	TEST_F(HookTest, EffectDepsGateReruns)
	{
		Reset();

		auto* s = AddSurface(
			{
				.Width = 100,
				.Height = 100,
				.Title = "deps",
			},
			[]() -> mocca::Element
			{
				mocca::useLayoutEffect(
					[]() -> void { C().Effects++; },
					mocca::deps()
				);
				return box(50, 50);
			}
		);

		Tick();
		EXPECT_EQ(C().Effects, 1);

		Tick();
		EXPECT_EQ(C().Effects, 1)
			<< "a clean surface skips Update";

		s->MarkDirty();
		Tick();
		EXPECT_EQ(C().Effects, 1)
			<< "unchanged deps must not re-run the effect";
	}
}