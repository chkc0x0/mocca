#include "text/FontMatcher.h"
#include "text/FontTypes.h"
#include "gtest/gtest.h"
#include <vector>

namespace mtest
{
	using mocca::FontStyleEnum;
	using mocca::Range;
	using mocca::mtext::FontDescriptor;
	using mocca::mtext::FontMatcher;
	using mocca::mtext::FontMatchResult;
	using mocca::mtext::FontRequest;

	namespace
	{
		auto Face(
			float weight,
			float stretch = 100.0F,
			FontStyleEnum style = FontStyleEnum::Normal
		) -> FontDescriptor
		{
			FontDescriptor d;
			d.Family = "Test";
			d.Weight = Range<float>(weight);
			d.Stretch = Range<float>(stretch);
			d.Style = style;
			return d;
		}

		auto Best(
			const std::vector<FontDescriptor>& faces,
			const FontRequest& q
		) -> FontMatchResult
		{
			std::vector<const FontDescriptor*> ptrs;
			ptrs.reserve(faces.size());
			for (const auto& f : faces)
			{
				ptrs.push_back(&f);
			}
			return FontMatcher::Best(ptrs, q);
		}
	}

	TEST(FontMatcher, EmptyCandidatesNoMatch)
	{
		auto r = Best({}, FontRequest{});

		EXPECT_FALSE(r);
	}

	TEST(FontMatcher, ExactWeightPicksFace)
	{
		auto r = Best({Face(400.0F)}, FontRequest{});

		ASSERT_TRUE(r);
		EXPECT_EQ(r.Index, 0U);
		EXPECT_FLOAT_EQ(r.Weight, 400.0F);
		EXPECT_FLOAT_EQ(r.Stretch, 100.0F);
	}

	TEST(FontMatcher, VariableRangeResolvesValue)
	{
		FontDescriptor variable = Face(400.0F);
		variable.Weight = Range<float>(100.0F, 900.0F);

		FontRequest q;
		q.Weight = 550.0F;

		auto r = Best({variable}, q);

		ASSERT_TRUE(r);
		EXPECT_FLOAT_EQ(r.Weight, 550.0F);
	}

	TEST(FontMatcher, WeightPrefersHigherFace)
	{
		FontRequest q;
		q.Weight = 450.0F;

		auto r = Best({Face(400.0F), Face(500.0F)}, q);

		ASSERT_TRUE(r);
		EXPECT_EQ(r.Index, 1U);
		EXPECT_FLOAT_EQ(r.Weight, 500.0F);
	}

	TEST(FontMatcher, WeightPrefersLowerFace)
	{
		FontRequest q;
		q.Weight = 300.0F;

		auto r = Best({Face(200.0F), Face(400.0F)}, q);

		ASSERT_TRUE(r);
		EXPECT_EQ(r.Index, 0U);
		EXPECT_FLOAT_EQ(r.Weight, 200.0F);
	}

	TEST(FontMatcher, NormalOverItalic)
	{
		auto r = Best(
			{Face(400.0F, 100.0F, FontStyleEnum::Italic), Face(400.0F)},
			FontRequest{}
		);

		ASSERT_TRUE(r);
		EXPECT_EQ(r.Index, 1U);
	}

	TEST(FontMatcher, ItalicObliqueOverNormal)
	{
		FontRequest q;
		q.Style = FontStyleEnum::Italic;

		auto r = Best(
			{Face(400.0F), Face(400.0F, 100.0F, FontStyleEnum::Oblique)},
			q
		);

		ASSERT_TRUE(r);
		EXPECT_EQ(r.Index, 1U);
	}

	TEST(FontMatcher, StretchPrefersNarrower)
	{
		FontRequest q;
		q.Stretch = 100.0F;

		auto r = Best({Face(400.0F, 75.0F), Face(400.0F, 125.0F)}, q);

		ASSERT_TRUE(r);
		EXPECT_EQ(r.Index, 0U);
	}

	TEST(FontMatcher, StretchPrefersWider)
	{
		FontRequest q;
		q.Stretch = 125.0F;

		auto r = Best({Face(400.0F, 100.0F), Face(400.0F, 150.0F)}, q);

		ASSERT_TRUE(r);
		EXPECT_EQ(r.Index, 1U);
	}

	TEST(FontMatcher, EqualRanksKeepFirstCandidate)
	{
		auto r = Best({Face(400.0F), Face(400.0F)}, FontRequest{});

		ASSERT_TRUE(r);
		EXPECT_EQ(r.Index, 0U);
	}

	namespace
	{
		auto VariableOblique(float lo, float hi) -> FontDescriptor
		{
			FontDescriptor f = Face(400.0F, 100.0F, FontStyleEnum::Oblique);
			f.Slant = Range<float>(lo, hi);
			return f;
		}

		auto ResolveSlant(const FontDescriptor& face, float requested) -> float
		{
			FontRequest q;
			q.Style = FontStyleEnum::Oblique;
			q.Slant = requested;

			auto r = Best({face}, q);
			EXPECT_TRUE(r);
			return r.Slant;
		}
	}

	TEST(FontMatcher, SlantWithinRangeValid)
	{
		EXPECT_FLOAT_EQ(
			ResolveSlant(VariableOblique(10.0F, 30.0F), 14.0F),
			14.0F
		);
	}

	TEST(FontMatcher, SlantClampsRange)
	{
		auto face = VariableOblique(10.0F, 30.0F);
		EXPECT_FLOAT_EQ(ResolveSlant(face, 5.0F), 10.0F);
		EXPECT_FLOAT_EQ(ResolveSlant(face, 40.0F), 30.0F);
	}

	TEST(FontMatcher, StaticObliqueResolves)
	{
		EXPECT_FLOAT_EQ(
			ResolveSlant(VariableOblique(20.0F, 20.0F), 14.0F),
			20.0F
		);
	}

	TEST(FontMatcher, UprightObliqueGetsZeroSlant)
	{
		auto r = Best({VariableOblique(0.0F, 0.0F)}, FontRequest{});

		ASSERT_TRUE(r);
		EXPECT_FLOAT_EQ(r.Slant, 0.0F);
	}

	TEST(FontMatcher, ObliqueUprightGetsZeroSlant)
	{
		FontDescriptor upright = Face(400.0F, 100.0F, FontStyleEnum::Normal);
		upright.Slant = Range<float>(10.0F, 30.0F);

		auto r = Best({upright}, FontRequest{.Style = FontStyleEnum::Oblique});

		ASSERT_TRUE(r);
		EXPECT_FLOAT_EQ(r.Slant, 0.0F);
	}
}
