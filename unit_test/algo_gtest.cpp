// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "ranges.h"
#include "dynarray.h"

#include "gtest/gtest.h"
#include <list>
#include <forward_list>
#include <deque>
#include <array>
#include <valarray>

namespace view = oel::view;

TEST(algoTest, eraseUnstable)
{
	using oel::erase_unstable;

	std::deque<std::string> d{"aa", "bb", "cc"};

	erase_unstable(d, 1);
	EXPECT_EQ(2U, d.size());
	EXPECT_EQ("cc", d.back());
	erase_unstable(d, 1);
	EXPECT_EQ(1U, d.size());
	EXPECT_EQ("aa", d.front());
}

TEST(algoTest, eraseIf)
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

TEST(algoTest, eraseAdjacentDup)
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

TEST(algoTest, copyUnsafe)
{
	std::valarray<int> src(2);
	src[0] = 1;
	src[1] = 2;
	std::valarray<int> dest(2);
	oel::copy_unsafe(src, begin(dest));
	EXPECT_EQ(1, dest[0]);
	EXPECT_EQ(2, dest[1]);
}

TEST(algoTest, copy)
{
	oel::dynarray<int> test = { 0, 1, 2, 3, 4 };
	int test2[5];
	constexpr int N = 4;
	test2[N] = -7;

OEL_WHEN_EXCEPTIONS_ON(
	EXPECT_THROW(oel::copy(test, view::counted(std::begin(test2), N)), std::out_of_range);
)
	auto success = oel::copy_fit(test, view::counted(std::begin(test2), N));
	EXPECT_TRUE(std::equal(begin(test), begin(test) + N, test2));
	EXPECT_EQ(-7, test2[N]);
	EXPECT_FALSE(success);

	ASSERT_EQ(4, test[N]);
	auto l = oel::copy(test2, test).source_last;
	EXPECT_EQ(-7, test[N]);
	EXPECT_TRUE(std::end(test2) == l);
	{
		std::list<std::string> li{"aa", "bb"};
		std::array<std::string, 2> strDest;
		auto sLast = oel::copy(view::move(li), strDest).source_last;
		EXPECT_EQ("aa", strDest[0]);
		EXPECT_EQ("bb", strDest[1]);
		EXPECT_TRUE(li.begin()->empty());
		EXPECT_TRUE(std::next(li.begin())->empty());
		EXPECT_TRUE(end(li) == sLast.base());
	}
	std::forward_list<std::string> li{"aa", "bb"};
	std::array<std::string, 4> strDest;
	success = oel::copy_fit(li, strDest);
	EXPECT_EQ("aa", strDest[0]);
	EXPECT_EQ("bb", strDest[1]);
	EXPECT_TRUE(success);
}

template<typename Container>
void testAppend()
{
	Container c;

	oel::append(c, std::initializer_list<int>{1, 2});
	EXPECT_EQ(2U, c.size());
	oel::append(c, 3, -1);
	EXPECT_EQ(5U, c.size());
	EXPECT_EQ( 2, *std::next(c.begin()) );
	EXPECT_EQ(-1, c.back());
}

TEST(algoTest, append)
{
	testAppend< std::list<int> >();
	testAppend< oel::dynarray<int> >();
}