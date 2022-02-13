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
#if OEL_STD_RANGES
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

	static_assert(std::is_trivially_constructible<CV, CV &>::value, "?");

#if OEL_STD_RANGES
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
	using Elem = std::unique_ptr<double>;
	Elem r[1];
	struct { auto operator()(const Elem &) /*mutable*/ { return 0; } } f;
	auto v = view::transform(r, f);
	auto v2 = view::transform(r, [](Elem & e) { return *e; });
	static_assert(std::is_same< decltype(v.begin())::iterator_category, std::forward_iterator_tag >(), "?");
	static_assert(std::is_same< decltype(v2.begin())::iterator_category, std::forward_iterator_tag >(), "?");
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

TEST(viewTest, viewTransformMutableLambda)
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