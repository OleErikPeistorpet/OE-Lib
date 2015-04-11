#include "util.h"
#include "iterator_range.h"
#include "dynarray.h"
#include "gtest/gtest.h"
#include <list>
#include <array>

using namespace oel;

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

TEST_F(utilTest, erase_successive_dup)
{
	std::list<int> li{1, 1, 2, 2, 2, 1, 3};
	dynarray<int> uniqueTest;
	uniqueTest.append(li);

	erase_successive_dup(li);
	EXPECT_EQ(4U, li.size());
	erase_successive_dup(uniqueTest);
	EXPECT_EQ(4U, uniqueTest.size());
}

TEST_F(utilTest, erase_back)
{
	std::list<int> li{1, 1, 2, 2, 2, 1, 3};
	erase_back(li, std::remove(begin(li), end(li), 1));
	EXPECT_EQ(4U, li.size());
}

TEST_F(utilTest, index_valid)
{
	std::list<std::string> li{"aa", "bb"};

	EXPECT_TRUE( index_valid(li, std::int64_t(1)) );
	EXPECT_FALSE(index_valid(li, 2));
	EXPECT_FALSE( index_valid(li, std::uint64_t(-1)) );

	//long l = 1L;
	//EXPECT_TRUE(index_valid(li, l));
}

TEST_F(utilTest, copy_nonoverlap)
{
	dynarray<int> test = { 0, 1, 2, 3, 4 };
	int test2[5];
	copy_nonoverlap(begin(test), count(test), test2);
	EXPECT_TRUE(std::equal(begin(test), end(test), test2));

	std::list<std::string> li{"aa", "bb"};
	std::array<std::string, 2> strDest;
	copy_nonoverlap( move_range(begin(li), end(li)), begin(strDest) );
	EXPECT_EQ("aa", strDest[0]);
	EXPECT_EQ("bb", strDest[1]);
	EXPECT_TRUE(li.front().empty());
	EXPECT_TRUE(li.back().empty());
}

TEST_F(utilTest, make_unique)
{
	auto p1 = make_unique<std::string[]>(5);
	for (size_t i = 0; i < 5; ++i)
		EXPECT_TRUE(p1[i].empty());

	auto p2 = make_unique<std::list<int>>(4, 6);
	EXPECT_EQ(4U, p2->size());
	EXPECT_EQ(6, p2->front());
	EXPECT_EQ(6, p2->back());
}
