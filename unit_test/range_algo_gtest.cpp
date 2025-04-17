// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "test_classes.h"
#include "range_algo.h"
#include "views.h"
#include "dynarray.h"

#include "gtest/gtest.h"
#include <list>
#include <forward_list>
#include <deque>
#include <array>
#include <valarray>
#include <string_view>

namespace view = oel::view;

TEST(rangeTest, unorderedErase)
{
	std::deque<std::string> d{"aa", "bb", "cc"};

	oel::unordered_erase(d, 1u);
	EXPECT_EQ(2U, d.size());
	EXPECT_EQ("cc", d.back());
	oel::unordered_erase(d, 1u);
	EXPECT_EQ(1U, d.size());
	EXPECT_EQ("aa", d.front());
}

template<typename Container>
void testUnorderedErase()
{
	Container c;
	c.emplace_back(-1);
	c.emplace_back(2);

	unordered_erase(c, 1);
	EXPECT_EQ(-1, *c.back());
	unordered_erase(c, 0);
	EXPECT_TRUE(c.empty());
}

TEST(rangeTest, unorderedEraseDynarray)
{
	testUnorderedErase< oel::dynarray<MoveOnly> >();
	testUnorderedErase< oel::dynarray<TrivialRelocat> >();
}

TEST(rangeTest, eraseIf)
{
	using namespace oel;

	std::list<int> li{1, 2, 3, 4, 5, 6};
	std::list<int> const expect{1, 3, 5};
	dynarray<int> test1;
	test1.append(li);

	auto isEven = [](int i) { return i % 2 == 0; };
	erase_if(li, isEven);
	EXPECT_TRUE(expect == li);
	erase_if(test1, isEven);
	EXPECT_EQ(li.size(), test1.size());
	EXPECT_TRUE(std::equal(begin(li), end(li), begin(test1)));
}

TEST(rangeTest, eraseAdjacentDup)
{
	using namespace oel;

	std::list<int> li{1, 1, 2, 2, 2, 1, 3};
	dynarray<int> const expect{1, 2, 1, 3};
	dynarray<int> uniqueTest;
	uniqueTest.assign(li);

	erase_adjacent_dup(li);
	EXPECT_EQ(4U, li.size());
	erase_adjacent_dup(uniqueTest);
	EXPECT_FALSE(uniqueTest != expect);
}

// TODO: test with allocator, verify only 1 allocation
TEST(rangeTest, concatToDynarray)
{
	using namespace std::string_view_literals;

	auto body = [] { return "Test"sv; };
	char const header[]{'v', '1', '\n'};

	auto result = oel::concat_to_dynarray(header, body());

	std::string_view v{result.data(), result.size()};
	EXPECT_EQ("v1\nTest"sv, v);
}

template<typename Container>
void testAppend()
{
	Container c;

	std::array<int, 2> const a{1, 7};
	oel::append(c, a);
	EXPECT_EQ(2U, c.size());
	EXPECT_EQ(7, c.back());
}

TEST(rangeTest, append)
{
#if __cpp_lib_containers_ranges
	testAppend< std::list<int> >();
#endif
	testAppend< std::basic_string<int> >();
	testAppend< oel::dynarray<int> >();
}
