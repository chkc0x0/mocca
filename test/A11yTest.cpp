#include "TestSupport.h"
#include "a11y/A11y.h"
#include "utils/TypeInterners.h"
#include <memory>

namespace mtest
{
	auto sampleProps() -> mocca::aria::Props
	{
		mocca::aria::Props props;
		props.Role = mocca::aria::Role::Button;
		props.Id = mocca::aria::id("save");
		props.Name = "Save";
		props.Selected = true;
		props.Level = 2;
		props.LabelledBy = {mocca::aria::id("label")};
		return props;
	}

	TEST(A11yProps, EqualityAndHash)
	{
		auto a = sampleProps();
		auto b = sampleProps();

		EXPECT_EQ(a, b);
		EXPECT_EQ(mocca::aria::PropsHash{}(a), mocca::aria::PropsHash{}(b));

		b.Name = "Save as";
		EXPECT_NE(a, b);

		b = sampleProps();
		b.LabelledBy.push_back(mocca::aria::id("hint"));
		EXPECT_NE(a, b);
	}

	TEST(A11yProps, SameA11yFallsBackToContent)
	{
		auto a = std::make_shared<mocca::aria::Props>(
			sampleProps()
		);
		auto b = std::make_shared<mocca::aria::Props>(
			sampleProps()
		);

		EXPECT_NE(a, b);
		EXPECT_TRUE(mocca::detail::sameA11y(a, b));
		EXPECT_FALSE(mocca::detail::sameA11y(a, nullptr));
		EXPECT_FALSE(mocca::detail::sameA11y(nullptr, b));
	}

	TEST(A11yProps, ConvenienceRoleAndName)
	{
		auto a = mocca::a11y(mocca::aria::Role::Button, "Save");
		auto b = mocca::a11y({
			.Role = mocca::aria::Role::Button,
			.Name = "Save",
		});

		EXPECT_TRUE(mocca::detail::sameA11y(a, b));
	}

	using A11yInternerTest = AppTest;

	TEST_F(A11yInternerTest, EqualPropsShareHandle)
	{
		auto a = mocca::a11y(sampleProps());
		auto b = mocca::a11y(sampleProps());

		EXPECT_EQ(a, b);
	}

	TEST_F(A11yInternerTest, DifferentPropsDiffer)
	{
		auto other = sampleProps();
		other.Name = "Cancel";

		auto a = mocca::a11y(sampleProps());
		auto b = mocca::a11y(other);

		EXPECT_NE(a, b);
		EXPECT_FALSE(mocca::detail::sameA11y(a, b));
	}

	TEST_F(A11yInternerTest, PruneDropsUnreferencedEntries)
	{
		std::weak_ptr<const mocca::aria::Props> weak;
		{
			const auto Handle = mocca::a11y(sampleProps());
			weak = Handle;
		}

		EXPECT_TRUE(weak.expired());

		App().GetInterners()->A11y.Prune();

		auto handle = mocca::a11y(sampleProps());
		EXPECT_TRUE(mocca::detail::sameA11y(handle, mocca::a11y(sampleProps())));
	}
}
