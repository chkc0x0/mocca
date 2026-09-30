#include "TestSupport.h"

namespace mtest
{
	TEST(ApplicationIdValidation, AcceptsValid)
	{
		EXPECT_TRUE(mocca::ApplicationID::ValidateID("two.parts"));
		EXPECT_TRUE(mocca::ApplicationID::ValidateID("three.parts.here"));
	}

	TEST(ApplicationIdValidation, RejectsInvalid)
	{
		EXPECT_FALSE(mocca::ApplicationID::ValidateID(""))
			<< "empty is not an id";
		EXPECT_FALSE(mocca::ApplicationID::ValidateID("nodots"))
			<< "one segment is not enough";
		EXPECT_FALSE(mocca::ApplicationID::ValidateID("four.parts.here.now"))
			<< "four segments is too many";
		EXPECT_FALSE(mocca::ApplicationID::ValidateID(".leading"))
			<< "a leading dot leaves an empty segment";
		EXPECT_FALSE(mocca::ApplicationID::ValidateID("trailing."))
			<< "a trailing dot leaves an empty segment";
		EXPECT_FALSE(mocca::ApplicationID::ValidateID("double..dot"))
			<< "a doubled dot leaves an empty segment";
	}

	TEST(ApplicationIdValidation, AcceptsCharacterSet)
	{
		EXPECT_TRUE(mocca::ApplicationID::ValidateID("with_underscores.and-dash"));
		EXPECT_TRUE(mocca::ApplicationID::ValidateID("Digits123.are4.fine5"));
		EXPECT_FALSE(mocca::ApplicationID::ValidateID("has space.part"))
			<< "whitespace is not allowed";
	}

	using ApplicationIdTest = AppTest;

	TEST_F(ApplicationIdTest, Accessors)
	{
		const auto& id = mocca::Application::GetAppID();

		EXPECT_EQ(id.Organization(), std::string_view("mocca"));
		EXPECT_EQ(id.Name(), std::string_view("test"));
		EXPECT_EQ(id.Domain(), std::string_view("com"));
	}

	TEST_F(ApplicationIdTest, GetCompoundId)
	{
		EXPECT_EQ(
			mocca::Application::main->GetAppID().GetCompoundID(),
			std::string("com.mocca.test")
		);
	}
}