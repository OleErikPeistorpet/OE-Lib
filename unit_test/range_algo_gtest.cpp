// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "range_algo.h"
#include "dynarray.h"

#include "gtest/gtest.h"
#include <list>
#include <forward_list>
#include <deque>
#include <array>
#include <valarray>
#include <functional>

namespace view = oel::view;

TEST(rangeTest, eraseUnstable)
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

TEST(rangeTest, countedView)
{
	using namespace oel;

	static_assert(std::is_nothrow_move_constructible<counted_view<int *>>::value, "?");

	dynarray<int> i{1, 2};
	counted_view<dynarray<int>::const_iterator> test = i;
	EXPECT_EQ(i.size(), test.size());
	EXPECT_TRUE(test.data() == i.data());
	EXPECT_EQ(1, test[0]);
	EXPECT_EQ(2, test[1]);
	test.drop_front();
	EXPECT_EQ(1U, test.size());
	EXPECT_EQ(2, test.back());
	EXPECT_TRUE(test.end() == i.end());
}

TEST(rangeTest, viewTransformBasics)
{
	using Elem = std::unique_ptr<double>;
	Elem r[1];
	struct { auto operator()(const Elem &) /*mutable*/ { return 0; } } f;
	auto v = view::transform(r, f);
	auto v2 = view::transform(r, [](Elem & e) { return *e; });
	static_assert(sizeof v.begin()  == sizeof(Elem *), "Not critical, this assert can be removed");
	static_assert(sizeof v2.begin() == sizeof(Elem *), "Not critical, this assert can be removed");
	{
		auto it = v.begin();
		EXPECT_TRUE( it++ == v.begin() );
		EXPECT_TRUE( it != v.begin() );
	}
	EXPECT_TRUE( v.begin() == v.begin().base() );
	EXPECT_FALSE( r + 1 == v.begin() );
	EXPECT_FALSE( v.begin() != r + 0 );
	EXPECT_TRUE( r + 1 != v.begin() );

	auto v3 = view::transform(v2, [](double d) { return d; });
	auto const it = v3.begin();
	EXPECT_TRUE(r + 0 == it);
	EXPECT_TRUE(r + 1 != it);
	EXPECT_FALSE(it == r + 1);
	EXPECT_FALSE(it != r + 0);
}

TEST(rangeTest, viewTransformSizedAndNonSizedRange)
{
	int src[] {1, 2};
	auto r = view::subrange(std::begin(src), std::end(src));
	oel::dynarray<int> test( view::transform(r, [](int & i) { return i++; }) );
	EXPECT_EQ(2U, test.size());
	EXPECT_EQ(1, test[0]);
	EXPECT_EQ(2, test[1]);
	EXPECT_EQ(2, src[0]);
	EXPECT_EQ(3, src[1]);

	struct Square
	{	int operator()(int i) const
		{
			return i * i;
		}
	};
	std::forward_list<int> const li{-2, -3};
	test.append( view::transform(li, Square{}) );
	EXPECT_EQ(4U, test.size());
	EXPECT_EQ(4, test[2]);
	EXPECT_EQ(9, test[3]);
}

TEST(rangeTest, viewTransformMutableLambda)
{
	auto iota = [i = 0](int) mutable
	{
		return i++;
	};
	int dummy[3];
	oel::dynarray<int> test(oel::reserve, 3);
	test.resize(1);

	test.assign( view::transform(dummy, iota) );
	EXPECT_EQ(0, test[0]);
	EXPECT_EQ(1, test[1]);
	EXPECT_EQ(2, test[2]);
}

TEST(rangeTest, viewTransformAsOutput)
{
	using Pair = std::pair<int, int>;
	Pair test[]{ {1, 2}, {3, 4} };

	auto f = [](Pair & p) -> int & { return p.second; };
	auto v = view::transform(test, std::function<int & (Pair &)>{f});
	*v.begin() = -1;
	v.drop_front();
	*v.begin() = -2;
	EXPECT_EQ(1, test[0].first);
	EXPECT_EQ(3, test[1].first);
	EXPECT_EQ(-1, test[0].second);
	EXPECT_EQ(-2, test[1].second);
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

TEST(rangeTest, append)
{
	testAppend< std::list<int> >();
	testAppend< oel::dynarray<int> >();
}
