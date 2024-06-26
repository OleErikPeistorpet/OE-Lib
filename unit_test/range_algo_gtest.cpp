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

TEST(rangeTest, copyUnsafe)
{
	std::valarray<int> src(2);
	src[0] = 1;
	src[1] = 2;
	std::valarray<int> dest(2);
	oel::copy_unsafe(src, begin(dest));
	EXPECT_EQ(1, dest[0]);
	EXPECT_EQ(2, dest[1]);
}

TEST(rangeTest, copy)
{
	oel::dynarray<int> test = { 0, 1, 2, 3, 4 };
	int test2[5];
	constexpr int N = 4;
	test2[N] = -7;

#if OEL_HAS_EXCEPTIONS
	EXPECT_THROW(oel::copy(test, view::counted(std::begin(test2), N)), std::out_of_range);
#endif
	auto success = oel::copy_fit(test, view::counted(std::begin(test2), N));
	EXPECT_TRUE(std::equal(begin(test), begin(test) + N, test2));
	EXPECT_EQ(-7, test2[N]);
	EXPECT_FALSE(success);

	ASSERT_EQ(4, test[N]);
	auto l = oel::copy(test2, test).in;
	EXPECT_EQ(-7, test[N]);
	EXPECT_TRUE(std::end(test2) == l);
	{
		std::list<std::string> li{"aa", "bb"};
		std::array<std::string, 2> strDest;
		auto sLast = oel::copy(view::move(li), strDest).in;
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

TEST(rangeTest, copyRangeMutableBeginSize)
{
	int src[1] {1};
	int dest[1];
	auto v = ToMutableBeginSizeView(src);

	oel::copy_unsafe(v, std::begin(dest));
	EXPECT_EQ(1, dest[0]);
	dest[0] = 0;

	oel::copy(v, dest);
	EXPECT_EQ(1, dest[0]);
	dest[0] = 0;

	oel::copy_fit(v, dest);
	EXPECT_EQ(1, dest[0]);
}

template<typename Container>
void testAppend()
{
	Container c;

	auto il = {1, 2};
	oel::append(c, il);
	EXPECT_EQ(2U, c.size());
	EXPECT_EQ(2, c.back());
}

TEST(rangeTest, append)
{
	testAppend< std::list<int> >();
	testAppend< oel::dynarray<int> >();
}
