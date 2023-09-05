// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "views.h"
#include "dynarray.h"
#include "util.h"

#include "gtest/gtest.h"
#include <array>
#include <forward_list>
#include <functional>
#include <sstream>

namespace view = oel::view;
using std::make_tuple;

constexpr auto transformIterFromIntPtr(const int * p)
{
	struct
	{	auto operator()(int i) const { return i; }
	} f;
	return oel::transform_iterator{f, p};
}

TEST(viewTest, subscript)
{
	std::array<int, 2> src{7, 8};
	auto v0 = view::counted(begin(src), ssize(src));
	auto v1 = view::subrange(begin(src), end(src));
	auto v2 = view::move(src);
	auto v3 = view::owning(std::move(src));

	EXPECT_EQ(7, v0[0]);
	EXPECT_EQ(8, v1[1]);
	EXPECT_EQ(7, v2[0]);
	EXPECT_EQ(8, v3[1]);
}

TEST(viewTest, nestedEmpty)
{
	oel::dynarray<int> src{};
	auto v = view::owning(view::subrange( begin(src), end(src) )) | view::move;
	EXPECT_TRUE(v.empty());
}

TEST(viewTest, viewSubrange)
{
	using V = view::subrange<int *, int *>;

	static_assert(std::is_trivially_constructible<V, V &>::value);

#if OEL_STD_RANGES
	static_assert(std::ranges::contiguous_range<V>);
	static_assert(std::ranges::view<V>);
	static_assert(std::ranges::borrowed_range<V>);
#endif
	static constexpr int src[3]{};
	{
		constexpr auto v = view::subrange(src + 1, src + 3);
		EXPECT_EQ(2, ssize(v));
	}
	constexpr auto it = transformIterFromIntPtr(src);
	constexpr auto v = view::subrange(it, make_tuple( std::end(src) ));
	EXPECT_EQ(3, ssize(v));
}

TEST(viewTest, viewCounted)
{
	using CV = view::counted<int *>;

	static_assert(std::is_trivially_constructible<CV, CV &>::value);

#if OEL_STD_RANGES
	static_assert(std::ranges::contiguous_range<CV>);
	static_assert(std::ranges::view<CV>);
	static_assert(std::ranges::borrowed_range<CV>);
#endif
	{
		oel::dynarray<int> i{1, 2};
		auto test = view::counted(i.begin(), i.size());
		EXPECT_EQ(i.size(), test.size());
		EXPECT_EQ(1, test[0]);
		EXPECT_EQ(2, test[1]);
		EXPECT_TRUE(test.end() == i.end());
	}
	static constexpr int src[1]{};
	constexpr auto v = view::counted(src, 1);
	EXPECT_EQ(std::end(src), v.end());
}

TEST(viewTest, viewTransformBasics)
{
	using Elem = double;

	struct MoveOnlyF
	{	std::unique_ptr<double> p;
		auto operator()(Elem &) const { return 0; }
	};
	Elem r[1];
	auto v = r | view::transform([](Elem &) { return 0; });
	auto v2 = view::transform(r, MoveOnlyF{});
	auto itMoveOnly = v2.begin();

	using IEmptyLambda = decltype(v.begin());
	using IMoveOnly = decltype(itMoveOnly);

	static_assert(std::is_same_v< IEmptyLambda::iterator_category, std::bidirectional_iterator_tag >);
	static_assert(std::is_same_v< IMoveOnly::iterator_category, std::input_iterator_tag >);
	static_assert(std::is_same_v< decltype(itMoveOnly++), void >);
	static_assert(sizeof(IEmptyLambda) == sizeof(Elem *), "Not critical, this assert can be removed");
#if OEL_STD_RANGES
	static_assert(std::ranges::bidirectional_range<decltype(v)>);
	static_assert(std::ranges::input_range<decltype(v2)>);
	{
		constexpr IEmptyLambda valueInit{};
		constexpr auto copy = valueInit;
	}
#endif
	{
		auto it = v.begin();
		EXPECT_TRUE( it++ == v.begin() );
		EXPECT_TRUE( it == v.end() );
		EXPECT_TRUE( --it == v.begin() );
		EXPECT_TRUE( (++it)-- != v.begin() );
	}
	EXPECT_TRUE( v.begin() == std::tuple{v.begin().base()} );
	EXPECT_FALSE( make_tuple(r + 1) == v.begin() );
	EXPECT_FALSE( v.begin() != make_tuple(r + 0) );
	EXPECT_TRUE( make_tuple(r + 1) != v.begin());

	auto nested = view::transform(v, [](double d) { return d; });
	auto const it = nested.begin();
	EXPECT_TRUE( make_tuple(make_tuple(r + 0)) == it );
	EXPECT_TRUE( make_tuple(make_tuple(r + 1)) != it );
	EXPECT_FALSE( it == make_tuple(make_tuple(r + 1)) );
	EXPECT_FALSE( it != make_tuple(make_tuple(r + 0)) );
}

using StdArrInt2 = std::array<int, 2>;

constexpr auto multBy2(StdArrInt2 a)
{
	StdArrInt2 res{};
	struct
	{	constexpr auto operator()(int i) const { return 2 * i; }
	} mult2;
	auto v = view::transform(a, mult2);
	std::ptrdiff_t i{};
	for (auto val : v)
		res[i++] = val;

	return res;
}

void testViewTransformConstexpr()
{
	constexpr StdArrInt2 a{1, 3};
	constexpr auto res = multBy2(a);
	static_assert(res[0] == 2);
	static_assert(res[1] == 6);
}


struct Square
{	int operator()(int i) const
	{
		return i * i;
	}
};

TEST(viewTest, viewTransformSizedRange)
{
	int src[] {1, 2};
	auto tv = src | view::transform([](int & i) { return i++; });
	auto tsr = view::subrange(tv.begin(), tv.end());
	oel::dynarray<int> dest(tsr);
	EXPECT_EQ(2U, tsr.size());
	EXPECT_EQ(2U, dest.size());
	EXPECT_EQ(1, dest[0]);
	EXPECT_EQ(2, dest[1]);
	EXPECT_EQ(2, src[0]);
	EXPECT_EQ(3, src[1]);

	std::forward_list<int> const li{-2};
	auto last = dest.append( view::counted(li.begin(), 1) | view::transform(Square{}) );
	EXPECT_EQ(3U, dest.size());
	EXPECT_EQ(4, dest[2]);
	EXPECT_EQ(li.end(), last.base());

	static_assert(std::is_same_v< decltype(last)::iterator_category, std::forward_iterator_tag >);
}

TEST(viewTest, viewTransformNonSizedRange)
{
	std::forward_list<int> const li{-2, -3};
	oel::dynarray<int> dest( view::transform(li, Square{}) );
	EXPECT_EQ(2U, dest.size());
	EXPECT_EQ(4, dest[0]);
	EXPECT_EQ(9, dest[1]);
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

TEST(viewTest, viewMoveEndDifferentType)
{
	auto nonEmpty = [i = -1](int j) { return i + j; };
	int src[1];
	oel::transform_iterator it{nonEmpty, src + 0};
	auto se = make_tuple(src + 1);
	auto v = view::subrange(it, se) | view::move;

	EXPECT_NE(v.begin(), v.end());
	EXPECT_EQ(se, v.end().base());
}

TEST(viewTest, viewEnumerate)
{
	double arr[] {1.1, 2.2};
	auto v = arr | view::move | view::enumerate;
	auto it = v.begin();

	static_assert(std::is_same_v< decltype(*it), std::pair<std::ptrdiff_t, double &&> >);
	EXPECT_EQ(2u, v.size());
	{
		auto && [i, e] = *it++;
		EXPECT_EQ(0, i);
		EXPECT_EQ(1.1, e);
	}
	auto && [i, e] = *it;
	EXPECT_EQ(1, i);
	EXPECT_EQ(2.2, e);
	EXPECT_EQ(++it, v.end());
}

#if OEL_STD_RANGES

static_assert(std::random_access_iterator< oel::iota_iterator<int> >);

void testEnableInfiniteRange()
{
	std::forward_list<int> li{};
	auto bounded = std::ranges::subrange(li);
	auto unbound = std::ranges::subrange(li.begin(), std::unreachable_sentinel);
	static_assert(not oel::enable_infinite_range< decltype(bounded) >);
	static_assert(oel::enable_infinite_range< decltype(unbound) >);
}

TEST(viewTest, viewMoveMutableEmptyAndSize)
{
	int src[] {0, 1};
	auto v = src | std::views::drop_while([](int i) { return i <= 0; }) | view::move;
	EXPECT_FALSE(v.empty());
	EXPECT_EQ(1U, v.size());
}

TEST(viewTest, chainWithStd)
{
	auto f = [](int i) { return -i; };
	int src[] {0, 1};

	src | view::move | std::views::drop_while([](int i) { return i <= 0; });
	src | std::views::reverse | view::transform(f) | std::views::take(1);
	src | view::transform(f) | std::views::drop(1) | view::move;
}

#endif
