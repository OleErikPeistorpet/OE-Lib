// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "views.h"
#include "dynarray.h"

#include "gtest/gtest.h"
#include <array>
#include <forward_list>
#include <functional>
#include <sstream>

namespace view = oel::view;

constexpr auto transformIterFromIntPtr(const int * p)
{
	struct F
	{
		auto operator()(int i) const { return i; }
	};
	return oel::transform_iterator<F, const int *>{F{}, p};
}

TEST(viewTest, basicView)
{
	using BV = oel::basic_view<int *>;

	static_assert(std::is_trivially_constructible<BV, BV &>::value, "?");

#if OEL_STD_RANGES
	static_assert(std::ranges::contiguous_range<BV>);
	static_assert(std::ranges::view<BV>);
	static_assert(std::ranges::borrowed_range<BV>);
#endif
	static constexpr int src[3]{};
	{
		constexpr auto v = view::subrange(src + 1, src + 3);
		EXPECT_EQ(2, ssize(v));
	}
	constexpr auto it = transformIterFromIntPtr(src);
	constexpr auto v = view::subrange(it, src + 3);
	EXPECT_EQ(3, ssize(v));
}

TEST(viewTest, countedView)
{
	using CV = oel::counted_view<int *>;

	static_assert(std::is_trivially_constructible<CV, CV &>::value, "?");

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

	struct MoveOnly
	{	std::unique_ptr<double> p;
		auto operator()(Elem &) const { return 0; }
	};
	Elem r[1];
	auto v = view::transform(r, [](Elem &) { return 0; });

	using IEmptyLambda = decltype(v.begin());
	using IMoveOnly = oel::transform_iterator<MoveOnly, Elem *>;

	static_assert(std::is_same< IEmptyLambda::iterator_category, std::bidirectional_iterator_tag >(), "?");
	static_assert(std::is_same< IMoveOnly::iterator_category, std::input_iterator_tag >(), "?");
	static_assert(sizeof(IEmptyLambda) == sizeof(Elem *), "Not critical, this assert can be removed");
#if OEL_STD_RANGES
	static_assert(std::ranges::bidirectional_range<decltype(v)>);
	static_assert(std::input_iterator<IMoveOnly>);
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
	EXPECT_TRUE( v.begin() == v.begin().base() );
	EXPECT_FALSE( r + 1 == v.begin() );
	EXPECT_FALSE( v.begin() != r + 0 );
	EXPECT_TRUE( r + 1 != v.begin() );

	auto nested = view::transform(v, [](double d) { return d; });
	auto const it = nested.begin();
	EXPECT_TRUE(r + 0 == it);
	EXPECT_TRUE(r + 1 != it);
	EXPECT_FALSE(it == r + 1);
	EXPECT_FALSE(it != r + 0);
}

#if __cplusplus > 201500 or _HAS_CXX17

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

#endif

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

	static_assert(std::is_same< decltype(last)::iterator_category, std::forward_iterator_tag >(), "?");
}

TEST(viewTest, viewTransformNonSizedRange)
{
	std::forward_list<int> const li{-2, -3};
	oel::dynarray<int> dest( view::transform(li, Square{}) );
	EXPECT_EQ(2U, dest.size());
	EXPECT_EQ(4, dest[0]);
	EXPECT_EQ(9, dest[1]);
}

TEST(viewTest, viewTransformMutableLambda)
{
	auto iota = [i = 0](int) mutable
	{
		return i++;
	};
	int dummy[3];
	auto v = view::transform(dummy, iota);
	using I = decltype(v.begin());

	static_assert(std::is_same<I::iterator_category, std::bidirectional_iterator_tag>(), "?");
#if OEL_STD_RANGES
	static_assert(std::input_or_output_iterator<I>);
	static_assert(std::ranges::range<decltype(v)>);
	{
		constexpr I valueInit{};
		constexpr auto copy = valueInit;
	}
#endif

	oel::dynarray<int> test(oel::reserve, 3);
	test.resize(1);

	test.assign(v);
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

TEST(viewTest, viewMoveEndDifferentType)
{
	auto nonEmpty = [i = -1](int j) { return i + j; };
	int src[1];
	oel::transform_iterator it{nonEmpty, src + 0};
	auto v = view::subrange(it, src + 1) | view::move();

	EXPECT_NE(v.begin(), v.end());
	EXPECT_EQ(src + 1, v.end().base());
}

#if OEL_STD_RANGES

TEST(viewTest, viewMoveMutableEmptyAndSize)
{
	int src[] {0, 1};
	auto v = src | std::views::drop_while([](int i) { return i <= 0; }) | view::move();
	EXPECT_FALSE(v.empty());
	EXPECT_EQ(1U, v.size());
}
#endif
