// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "range_view.h"
#include "dynarray.h"

#include "gtest/gtest.h"
#include <forward_list>
#include <functional>

namespace view = oel::view;
using oel::basic_view;
using oel::counted_view;

auto transformIterFromIntPtr(int * p)
{
	auto f = [](int i) { return i; };
	return oel::transform_iterator<decltype(f), int *>{f, p};
}

TEST(viewTest, basicView)
{
#if OEL_HAS_STD_RANGES
	using BV = oel::basic_view<int *>;

	static_assert(std::ranges::contiguous_range<BV>);
	static_assert(std::ranges::view<BV>);
	static_assert(std::ranges::borrowed_range<BV>);
#endif
	int src[3]{};
	{
		basic_view<const int *> v{std::begin(src), std::end(src)};
		EXPECT_EQ(3, ssize(v));
	}
	auto it = transformIterFromIntPtr(src);
	basic_view<decltype(it), int *> v{it, std::end(src)};
	EXPECT_EQ(3, ssize(v));
}

TEST(viewTest, countedView)
{
	using CV = counted_view<int *>;

	static_assert(std::is_nothrow_move_constructible<CV>::value, "?");

#if OEL_HAS_STD_RANGES
	static_assert(std::ranges::contiguous_range<CV>);
	static_assert(std::ranges::view<CV>);
	static_assert(std::ranges::borrowed_range<CV>);
#endif
	{
		oel::dynarray<int> i{1, 2};
		counted_view<oel::dynarray<int>::const_iterator> test = i;
		EXPECT_EQ(i.size(), test.size());
		EXPECT_EQ(1, test[0]);
		EXPECT_EQ(2, test[1]);
		test.drop_front();
		EXPECT_EQ(1U, test.size());
		EXPECT_EQ(2, test.back());
		EXPECT_TRUE(test.end() == i.end());
	}
	int src[1]{};
	auto it = transformIterFromIntPtr(src);
	counted_view<decltype(it)> v{it, 1};
	EXPECT_EQ(std::end(src), v.end());
}

TEST(viewTest, viewTransformBasics)
{
	using Elem = oel::dynarray<int>;
	Elem r[1];
	auto v = view::transform(r, [](const Elem & c) { return c.size(); });
	static_assert( std::is_same< decltype(v.begin())::iterator_category, std::forward_iterator_tag >{},
		"Wrong for current implementation" );
	static_assert( sizeof v.begin() == sizeof(Elem *),
		"Not critical, this assert can be removed" );

	EXPECT_TRUE( v.begin() == r + 0 );
	EXPECT_FALSE( r + 1 == v.begin() );
	EXPECT_FALSE( v.begin() != r + 0 );
	EXPECT_TRUE( r + 1 != v.begin() );
}

TEST(viewTest, viewTransformSizedAndNonSizedRange)
{
	int src[] {1, 2};
	auto r = view::subrange(std::begin(src), std::end(src));
	oel::dynarray<int> dest( view::transform(r, [](int & i) { return i++; }) );
	EXPECT_EQ(2U, dest.size());
	EXPECT_EQ(1, dest[0]);
	EXPECT_EQ(2, dest[1]);
	EXPECT_EQ(2, src[0]);
	EXPECT_EQ(3, src[1]);

	struct Square
	{	int operator()(int i) const
		{
			return i * i;
		}
	};
	std::forward_list<int> const li{-2, -3};
	dest.append( view::transform(li, Square{}) );
	EXPECT_EQ(4U, dest.size());
	EXPECT_EQ(4, dest[2]);
	EXPECT_EQ(9, dest[3]);
}

TEST(viewTest, viewTransformAsOutput)
{
	using Pair = std::pair<int, int>;
	Pair test[]{ {1, 2}, {3, 4} };

	auto f = [](Pair & p) -> int & { return p.second; };
	auto v = view::transform(test, std::function<int & (Pair &)>{f});
	int n{};
	auto const last = v.end();
	for (auto it = v.begin(); it != last; ++it)
		*it = --n;

	EXPECT_EQ(1, test[0].first);
	EXPECT_EQ(3, test[1].first);
	EXPECT_EQ(-1, test[0].second);
	EXPECT_EQ(-2, test[1].second);
}