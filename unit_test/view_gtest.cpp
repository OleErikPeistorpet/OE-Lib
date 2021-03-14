// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "test_classes.h"
#include "ranges.h"
#include "dynarray.h"

#include "gtest/gtest.h"
#include <forward_list>
#include <functional>

namespace view = oel::view;

TEST(viewTest, countedView)
{
	using namespace oel;

	static_assert(std::is_nothrow_move_constructible<counted_view<int *>>::value, "?");

#if OEL_HAS_RANGES
	static_assert(std::ranges::contiguous_range< counted_view<int *> >);
	static_assert(std::ranges::view< counted_view<int *> >);
#endif

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
	EXPECT_TRUE( r + 0 == v.begin() );
	EXPECT_TRUE( v.begin() != r + 1 );
	EXPECT_TRUE( r + 1 != v.begin() );
}

TEST(viewTest, viewTransformSizedAndNonSizedRange)
{
	struct Square
	{	int operator()(int i) const
		{
			return i * i;
		}
	};
	int src[] {2, 3};
	auto r = view::subrange(std::begin(src), std::end(src));
	oel::dynarray<int> test( view::transform(r, Square{}) );
	EXPECT_EQ(2U, test.size());
	EXPECT_EQ(4, test[0]);
	EXPECT_EQ(9, test[1]);

	std::forward_list<int> li{ 1, 2 };
	test.append( view::transform(li, [](int & i) { return i++; }) );
	EXPECT_EQ(4U, test.size());
	EXPECT_EQ(1, test[2]);
	EXPECT_EQ(2, test[3]);
	EXPECT_EQ(2, src[0]);
	EXPECT_EQ(3, src[1]);
}

TEST(viewTest, viewTransformAsOutput)
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