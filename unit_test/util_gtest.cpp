#include "util.h"
#include "iterator_range.h"
#include "dynarray.h"
#include "gtest/gtest.h"
#include <list>
#include <array>

/// @cond INTERNAL

// The fixture for testing algorithms and utilities
class utilTest : public ::testing::Test
{
protected:
	utilTest()
	{
		// You can do set-up work for each test here.
	}

	// Objects declared here can be used by all tests.
};

TEST_F(utilTest, eraseUnordered)
{
	using namespace oel;

	dynarray<std::string> da{"aa", "bb", "cc"};
	{
		std::list<std::string> li(begin(da), end(da));

		auto it = begin(li);
		erase_unordered(li, ++it);
		EXPECT_EQ(2U, li.size());
		EXPECT_EQ("cc", li.back());
		erase_unordered(li, it);
		EXPECT_EQ(1U, li.size());
		EXPECT_EQ("aa", li.front());
	}
	erase_unordered(da, 1);
	EXPECT_EQ(2U, da.size());
	EXPECT_EQ("cc", da.back());
	erase_unordered(da, 1);
	EXPECT_EQ(1U, da.size());
	EXPECT_EQ("aa", da.front());
}

TEST_F(utilTest, eraseSuccessiveDup)
{
	using namespace oel;

	std::list<int> li{1, 1, 2, 2, 2, 1, 3};
	dynarray<int> uniqueTest;
	uniqueTest.append(li);

	erase_successive_dup(li);
	EXPECT_EQ(4U, li.size());
	erase_successive_dup(uniqueTest);
	EXPECT_EQ(4U, uniqueTest.size());
}

TEST_F(utilTest, eraseBack)
{
	std::list<int> li{1, 1, 2, 2, 2, 1, 3};
	oel::erase_back(li, std::remove(begin(li), end(li), 1));
	EXPECT_EQ(4U, li.size());
}

TEST_F(utilTest, indexValid)
{
	using namespace oel;

	std::list<std::string> li{"aa", "bb"};

	EXPECT_TRUE( index_valid(li, std::int64_t(1)) );
	EXPECT_FALSE(index_valid(li, 2));
	EXPECT_FALSE( index_valid(li, std::uint64_t(-1)) );

	//long l = 1L;
	//EXPECT_TRUE(index_valid(li, l));
}

TEST_F(utilTest, copyNonoverlap)
{
	oel::dynarray<int> test = { 0, 1, 2, 3, 4 };
	int test2[5];
	oel::copy_nonoverlap(begin(test), ssize(test), adl_begin(test2));
	EXPECT_TRUE(std::equal(begin(test), end(test), test2));

	std::list<std::string> li{"aa", "bb"};
	std::array<std::string, 2> strDest;
	oel::copy_nonoverlap( oel::move_range(begin(li), end(li)), begin(strDest) );
	EXPECT_EQ("aa", strDest[0]);
	EXPECT_EQ("bb", strDest[1]);
	EXPECT_TRUE(li.front().empty());
	EXPECT_TRUE(li.back().empty());
}

TEST_F(utilTest, makeUnique)
{
	auto p1 = oel::make_unique<std::string[]>(5);
	for (int i = 0; i < 5; ++i)
		EXPECT_TRUE(p1[i].empty());

	auto p2 = oel::make_unique<std::list<int>>(4, 6);
	EXPECT_EQ(4U, p2->size());
	EXPECT_EQ(6, p2->front());
	EXPECT_EQ(6, p2->back());
}

struct BlahBy
{
	using difference_type = long;

	unsigned short size() const  { return 2; }
};

TEST_F(utilTest, ssize)
{
	BlahBy bb;
	auto const test = oel::ssize(bb);

	static_assert(std::is_same<decltype(test), long const>::value, "?");

	auto v = std::is_same<short, decltype( oel::as_signed(bb.size()) )>::value;
	EXPECT_TRUE(v);

	ASSERT_EQ(2, test);
}

/// @endcond
