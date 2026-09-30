#include "TestSupport.h"

namespace mtest
{
	using LifecycleTest = LifecycleTestBase;

	TEST_F(LifecycleTest, SurfaceDiesNextFrame)
	{
		auto [parent, child] = makeParentChildTree(*this);
		Watch.Watch(child);

		child->RequestClose();
		EXPECT_EQ(child->GetState(), mocca::SurfaceState::Zombie);

		Tick();
		EXPECT_EQ(child->GetState(), mocca::SurfaceState::Dead)
			<< "surface should be closed";

		Tick();
		EXPECT_EQ(Watch.Count(), 1) << "the parent must announce its dead child";
		(void)parent;
	}

	TEST_F(LifecycleTest, SurfaceDeadDelayed)
	{
		auto [parent, child] = makeParentChildTree(*this);
		Watch.Watch(child);

		child->SetZombieTimeout(3);
		child->RequestClose();
		ASSERT_EQ(child->GetState(), mocca::SurfaceState::Zombie);

		Tick(2);
		EXPECT_EQ(child->GetState(), mocca::SurfaceState::Zombie)
			<< "the surface must survive";
		EXPECT_EQ(Watch.Count(), 0);

		Tick(2);
		EXPECT_EQ(Watch.Count(), 1) << "the surface must die";
		(void)parent;
	}

	TEST_F(LifecycleTest, SurfaceTimeoutPostClose)
	{
		auto [parent, child] = makeParentChildTree(*this);
		Watch.Watch(child);

		child->RequestClose();
		child->SetZombieTimeout(3);

		Tick(2);
		EXPECT_EQ(child->GetState(), mocca::SurfaceState::Zombie)
			<< "the surface must survive";
		EXPECT_EQ(Watch.Count(), 0);

		Tick(2);
		EXPECT_EQ(Watch.Count(), 1) << "the surface must die";
		(void)parent;
	}

	TEST_F(LifecycleTest, SurfaceForceDestroy)
	{
		auto [parent, child] = makeParentChildTree(*this);
		Watch.Watch(child);

		child->SetZombieTimeout(1000);
		child->ForceDestroy();

		Tick();
		EXPECT_EQ(Watch.Count(), 1)
			<< "ForceDestroy must not wait";
		(void)parent;
	}
}