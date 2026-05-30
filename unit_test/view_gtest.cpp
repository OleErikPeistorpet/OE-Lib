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

#if OEL_STD_RANGES
	#if defined __clang__ and __clang_major__ < 16
	#define STD_VIEW_REQUIRES_DEFAULT_CONSTRUCT  1
	#else
	#define STD_VIEW_REQUIRES_DEFAULT_CONSTRUCT  0
	#endif
#endif

TEST(viewTest, viewAll)
{
	using FL = std::forward_list<int>;
	using Sized = std::array<int, 2>;

	{	auto v = view::all(FL{7});
		static_assert(std::is_same_v< decltype(v), view::owning<FL> >);

		auto v2 = view::all(std::move(v));
		static_assert(std::is_same_v< decltype(v), decltype(v2) >);

		auto c = std::move(v2).base();
		EXPECT_EQ(7, *c.begin());
		EXPECT_TRUE(v2.empty());
	}
	{	Sized c{7, 8};
		auto v = view::all(c);
	#if OEL_STD_RANGES
		static_assert(std::is_same_v< decltype(v), std::ranges::ref_view<Sized> >);
	#else
		using I = Sized::iterator;
		static_assert(std::is_same_v< decltype(v), oel::_countedView<I> >);
	#endif
		auto v2 = view::all(std::as_const(v));
		static_assert(std::is_same_v< decltype(v), decltype(v2) >);

		EXPECT_EQ(&c[0], &v2[0]);
		EXPECT_EQ(&c[1], &v2[1]);
	}
	{	FL const c{7, 8};
		auto v = view::all(c);
		// Should hit static_assert
		//auto ov = view::owning(std::move(c));
	#if OEL_STD_RANGES
		static_assert(std::is_same_v< decltype(v), std::ranges::ref_view<FL const> >);
	#else
		using I = FL::const_iterator;
		static_assert(std::is_same_v< decltype(v), view::subrange<I, I> >);
	#endif
		auto v2 = view::all(v);
		static_assert(std::is_same_v< decltype(v), decltype(v2) >);

		EXPECT_EQ(c.begin(), v2.begin());
		EXPECT_EQ(c.end(), v2.end());
	}
}

constexpr auto transformIterFromIntPtr(const int * p)
{
	struct F
	{
		auto operator()(int i) const { return i; }
	};
	return oel::transform_iterator<F, const int *>{F{}, p};
}

template< typename S >
constexpr oel::sentinel_wrapper<S> makeSentinel(S se) { return {se}; }

TEST(viewTest, subscript)
{
	std::array<int, 2> src{7, 8};
	auto v0 = view::counted(begin(src), oel::ssize(src));
	auto v1 = view::subrange(begin(src), end(src));
	auto v2 = view::move(src);
	auto v3 = view::owning(std::move(src));

	EXPECT_EQ(7, v0[0]);
	EXPECT_EQ(8, v1[1]);
#if !OEL_STD_RANGES or __cpp_lib_move_iterator_concept >= 202207
	EXPECT_EQ(7, v2[0]);
#endif
	EXPECT_EQ(8, v3[1]);
}

TEST(viewTest, nestedEmpty)
{
	oel::dynarray<int> src{};
	auto v = view::owning(view::subrange( src.begin(), src.end() )) | view::move;
	EXPECT_TRUE(v.empty());
	EXPECT_TRUE(!v);
}

TEST(viewTest, viewSubrange)
{
	using V = view::subrange<int *, int *>;

	static_assert(std::is_trivially_constructible<V, V &>::value);

#if OEL_STD_RANGES
	static_assert(std::ranges::contiguous_range<V>);
	static_assert(std::ranges::borrowed_range<V>);
	#if !STD_VIEW_REQUIRES_DEFAULT_CONSTRUCT
	static_assert(std::ranges::view<V>);
	#endif
#endif
#if __cpp_lib_concepts
	static_assert(
		sizeof( view::subrange<int *, std::default_sentinel_t> ) == sizeof(int *),
		"Not critical, this assert can be removed" );
#endif
	static constexpr int src[3]{};
	{
		constexpr auto v = view::subrange(src + 1, src + 3);
		EXPECT_EQ(2, ssize(v));
	}
	constexpr auto it = transformIterFromIntPtr(src);
	constexpr auto v = view::subrange(it, makeSentinel(src + 3));
	EXPECT_EQ(3, ssize(v));
}

TEST(viewTest, viewCounted)
{
	using CV = oel::_countedView<int *>;

	static_assert(std::is_trivially_constructible<CV, CV &>::value);

#if OEL_STD_RANGES
	static_assert(std::ranges::contiguous_range<CV>);
	static_assert(std::ranges::borrowed_range<CV>);
	#if !STD_VIEW_REQUIRES_DEFAULT_CONSTRUCT
	static_assert(std::ranges::view<CV>);
	#endif
#endif
	{
		oel::dynarray<int> i{1, 2};
		auto test = view::counted(i.begin(), ssize(i));
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
	auto v = view::transform(r, [](Elem &) { return 0; });
	auto v2 = view::transform(r, MoveOnlyF{});
	auto itMoveOnly = v2.begin();

	using IEmptyLambda = decltype(v.begin());

	static_assert(std::is_same_v< IEmptyLambda::iterator_category, std::random_access_iterator_tag >);
	static_assert(std::is_same_v< decltype(itMoveOnly)::iterator_category, std::input_iterator_tag >);
	static_assert(std::is_same_v< decltype(itMoveOnly++), void >);
	static_assert(sizeof(IEmptyLambda) == sizeof(Elem *));
#if OEL_STD_RANGES
	static_assert(std::ranges::random_access_range< decltype(v) >);
	static_assert(std::ranges::input_range< decltype(v2) >);
	static_assert(std::ranges::view< decltype(v) >);
	using RVal = decltype( std::move(v) );
	static_assert(std::ranges::borrowed_range<RVal>);
	{
		constexpr IEmptyLambda valueInit{};
		[[maybe_unused]] constexpr auto copy = valueInit;
	}
#endif
	{
		auto it = v.begin();
		EXPECT_FALSE( it >= v.end() );

		EXPECT_TRUE( it++ == v.begin() );
		EXPECT_TRUE( it == v.end() );
		EXPECT_TRUE( it > v.begin() );
		EXPECT_FALSE( it <= v.begin() );

		EXPECT_TRUE( --it == v.begin() );
		EXPECT_TRUE( (++it)-- != v.begin() );
	}
	EXPECT_TRUE( v.begin() == makeSentinel(v.begin().base()) );
	EXPECT_FALSE( makeSentinel(r + 1) == v.begin() );
	EXPECT_FALSE( v.begin() != makeSentinel(r + 0) );
	EXPECT_TRUE( makeSentinel(r + 1) != v.begin() );

	auto nested = view::transform(v, [](double d) { return d; });
	auto const it = nested.begin();
	EXPECT_TRUE(makeSentinel(r + 0) == it.base());
	EXPECT_TRUE(makeSentinel(r + 1) != it.base());
	EXPECT_FALSE(it.base() == makeSentinel(r + 1));
	EXPECT_FALSE(it.base() != makeSentinel(r + 0));
}

using StdArrInt2 = std::array<int, 2>;

constexpr auto multBy2(StdArrInt2 a)
{
	StdArrInt2 res{};
	auto mult2 = [j = 2](int i) { return i * j; };
	auto v = view::transform(a, mult2);

	size_t i{};
#if __cplusplus >= 202001 or (defined _MSVC_STL_VERSION and _HAS_CXX20)
	auto src = v.begin();
	decltype(src) it2;
	res[i++] = *src++;
	it2 = src;
	res[i] = *it2;
#else
	for (auto val : v)
		res[i++] = val;
#endif

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
	EXPECT_EQ(1, tv[0]);
	EXPECT_EQ(2, tv[1]);

	auto tsr = view::subrange(tv.begin(), tv.end());
	auto dest = tsr | oel::to_dynarray();
	EXPECT_EQ(2U, tsr.size());
	EXPECT_EQ(2U, dest.size());
	EXPECT_EQ(2, dest[0]);
	EXPECT_EQ(3, dest[1]);
	EXPECT_EQ(3, src[0]);
	EXPECT_EQ(4, src[1]);

	std::forward_list<int> const li{-2};
	dest.append_range( view::counted(li.begin(), 1) | view::transform(Square{}) );
	EXPECT_EQ(3U, dest.size());
	EXPECT_EQ(4, dest[2]);
}

TEST(viewTest, viewTransformNonSizedRange)
{
	std::forward_list<int> const li{-2, -3};
	auto dest = view::transform(li, Square{}) | oel::to_dynarray();
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
	static_assert(std::is_same_v<I::iterator_category, std::input_iterator_tag>);
#if OEL_STD_RANGES
	static_assert(!std::forward_iterator<I>);
#endif

	oel::dynarray<int> test(oel::reserve, 3);
	test.resize(1);

	test.assign_range(v);
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
	auto it = v.begin();
	for (ptrdiff_t i{}; i != ssize(v); ++i)
		it[i] = --n;

	EXPECT_EQ(1, test[0].first);
	EXPECT_EQ(3, test[1].first);
	EXPECT_EQ(-1, test[0].second);
	EXPECT_EQ(-2, test[1].second);
}

struct MoveOnlyIterWithForwardTag
{
	using iterator_category = std::forward_iterator_tag;
	using value_type        = int;
	using reference         = int;
	using pointer           = void;
	using difference_type   = ptrdiff_t;

	constexpr int operator*() const { return 7; }

	MoveOnlyIterWithForwardTag() = default;
	MoveOnlyIterWithForwardTag(MoveOnlyIterWithForwardTag &&)      = default;
	MoveOnlyIterWithForwardTag(const MoveOnlyIterWithForwardTag &) = delete;
	MoveOnlyIterWithForwardTag& operator =(MoveOnlyIterWithForwardTag &&) = default;

	constexpr MoveOnlyIterWithForwardTag & operator++()      { return *this; }
	constexpr void                         operator++(int) & {}
};

TEST(viewTest, testTransformMoveOnlyIterator)
{
	auto v = view::counted(MoveOnlyIterWithForwardTag{}, 1)
			| view::transform([](int i) { return i; });
	auto it = v.begin();
	static_assert(std::is_same_v< decltype(it++), void >);
	EXPECT_EQ(7, *it);
}

#if __cpp_lib_concepts

struct IterWithConceptOnly
{
	using iterator_concept = std::forward_iterator_tag;
	using value_type       = int;
	using difference_type  = ptrdiff_t;

	constexpr int operator*() const { return 7; }

	constexpr IterWithConceptOnly & operator++()      { return *this; }
	constexpr void                  operator++(int) & {}
};

void testTransformIterWithConceptOnly()
{
	auto v = view::counted(IterWithConceptOnly{}, 1)
			| view::transform([](int i) { return i; });
	auto it = v.begin();
	using I = decltype(it);
	static_assert(std::is_same_v< I::iterator_category, std::forward_iterator_tag >);
	static_assert(std::is_same_v< decltype(it++), I >);
}
#endif

#if !defined __GLIBCXX__ or __GNUC__ > 10 or !__cpp_lib_concepts

constexpr StdArrInt2 generatedArray()
{
	StdArrInt2 res{};
	int i{1};
	auto v = view::generate([&i] { return i++; }, 2);
	if (v)
	{
		auto it = v.begin();
		for (auto & val : res)
		{
			val = *it;
			it++;
		}
	}
	return res;
}

constexpr void testViewGenerateConstexpr()
{
	constexpr auto res = generatedArray();
	static_assert(res[0] == 1);
	static_assert(res[1] == 2);
}
#endif

struct Ints
{
	int i;

	int operator()()  { return i++; }
};

TEST(viewTest, viewGenerate)
{
	auto d = view::generate(Ints{1}, 2) | oel::to_dynarray();

	ASSERT_EQ(2U, d.size());
	EXPECT_EQ(1, d[0]);
	EXPECT_EQ(2, d[1]);

	d.assign_range(oel::view::generate(Ints{}, 0));
	EXPECT_TRUE(d.empty());

#if OEL_STD_RANGES
	auto v = view::generate([] { return 7; }, 1);
	for (auto i : v)
		EXPECT_EQ(7, i);

	static_assert(sizeof v <= 2 * sizeof(ptrdiff_t));

	using V = decltype(v);
	static_assert(std::ranges::input_range<V>);
	static_assert(std::ranges::sized_range<V>);
#endif
}

#if __cpp_lib_concepts
TEST(viewTest, viewGenerateUnbounded)
{
	auto v = view::generate([i = 6]() mutable{ return i++; });
	auto it = v.begin();
	EXPECT_EQ(6, *it);
	it++;
	EXPECT_EQ(7, *it);
}
#endif

TEST(viewTest, viewMoveEndDifferentType)
{
	auto nonEmpty = [i = -1](int j) { return i + j; };
	int src[1];
	oel::transform_iterator it{nonEmpty, src + 0};
	auto v = view::subrange(it, makeSentinel(src + 1)) | view::move;

#if OEL_STD_RANGES
	static_assert(std::ranges::enable_borrowed_range< decltype(v) >);
#endif
	static_assert(oel::range_is_sized<decltype(v)>);
	EXPECT_NE(v.begin(), v.end());
	EXPECT_EQ(src + 1, v.end().base().se);
}

void testStdIteratorInOelViewNotAmbiguous()
{
	auto v0 = std::array<int, 1>{} | view::move;
	auto v  = view::subrange(v0.begin(), v0.end());
	static_assert(
		std::is_same_v<
			oel::iterator_t< decltype(v) >,
			oel::sentinel_t< decltype(v) >
		> );
}

#if OEL_STD_RANGES

TEST(viewTest, viewMoveMutableEmptyAndSize)
{
	int src[] {0, 1};
	auto v = src | std::views::drop_while([](int i) { return i <= 0; }) | view::move;
	EXPECT_FALSE(v.empty());
	EXPECT_EQ(1U, v.size());
}

#if !STD_VIEW_REQUIRES_DEFAULT_CONSTRUCT
TEST(viewTest, chainWithStd)
{
	auto f = [](int i) { return -i; };
	int src[] {0, 7};

	void( src | view::move | std::views::drop_while([](int i) { return i <= 0; }) );

	auto v = src | std::views::reverse | view::transform(f) | std::views::take(1);
	EXPECT_EQ(-7, v[0]);

	void( src | view::transform(f) | std::views::drop(1) | view::move );
}
#endif
#endif
