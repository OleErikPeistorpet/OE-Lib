// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "test_classes.h"
#include "dynarray.h"
#include "optimize_ext/std_tuple.h"
#include "optimize_ext/std_variant.h"

#include "gtest/gtest.h"
#include <list>
#include <set>
#include <array>
#if HAS_STD_PMR
#include <memory_resource>
#endif

namespace
{
	struct NonTrivialDestruct
	{
		~NonTrivialDestruct() { ; }
	};

	static_assert(oel::is_trivially_relocatable< std::pair<int *, std::unique_ptr<int>> >::value);
	static_assert(oel::is_trivially_relocatable< std::tuple<std::unique_ptr<double>> >::value);
	static_assert(oel::is_trivially_relocatable< std::tuple<> >::value);
	static_assert( !oel::is_trivially_relocatable< std::tuple<int, NonTrivialDestruct, int> >::value );

	static_assert(oel::is_trivially_relocatable< std::variant< std::unique_ptr<double>, int > >::value);
	static_assert( !oel::is_trivially_relocatable< std::variant<NonTrivialDestruct> >::value );

#if (defined _CPPLIB_VER or defined _LIBCPP_VERSION or defined __GLIBCXX__) and !_GLIBCXX_USE_CXX11_ABI
	static_assert(oel::is_trivially_relocatable< std::string >::value);
#endif
#if OEL_HAS_BOOST
	static_assert(oel::is_trivially_relocatable< boost::circular_buffer<int, oel::allocator<int>> >());
#endif

	struct alignas(32) Foo { int a[24]; };
	static_assert(alignof(oel::_detail::RelocateWrap<Foo>) == 32);
	static_assert(sizeof(oel::_detail::RelocateWrap<Foo>) == sizeof(Foo));

	using ListI = std::list<int>::iterator;
	static_assert(!oel::can_memmove_with< int *, float * >);
	static_assert(!oel::can_memmove_with< int *, std::set<int>::iterator >);
	static_assert(!oel::can_memmove_with< int *, std::move_iterator<ListI> >);
	static_assert(oel::can_memmove_with< std::array<int, 1>::iterator, std::move_iterator<const int *> >);

	static_assert(!oel::iter_is_random_access<ListI>);
}

TEST(utilTest, ForwardT)
{
	using oel::_detail::ForwardT;

	static_assert(std::is_same_v< ForwardT<double const>, double >);
	static_assert(std::is_same_v< ForwardT<double &&>, double >);
	static_assert(std::is_same_v< ForwardT<const double &&>, double >);
	static_assert(std::is_same_v< ForwardT<const double &>, double >);
	static_assert(std::is_same_v< ForwardT<double &>, double & >);

	static_assert(std::is_same_v< ForwardT<int[1]>, int(&&)[1] >);
	static_assert(std::is_same_v< ForwardT<int(&&)[1]>, int(&&)[1] >);
	static_assert(std::is_same_v< ForwardT<int(&)[1]>, int(&)[1] >);
	static_assert(std::is_same_v< ForwardT<const int(&)[1]>, const int(&)[1] >);

	using P = std::unique_ptr<int>;
	static_assert(std::is_same_v< ForwardT<P const>, const P && >);
	static_assert(std::is_same_v< ForwardT<const P &&>, const P && >);
	static_assert(std::is_same_v< ForwardT<P &>, P & >);
	static_assert(std::is_same_v< ForwardT<const P &>, const P & >);

	// Small, non-trivial copy
	static_assert(std::is_same_v< ForwardT<TrivialRelocat const>, const TrivialRelocat && >);
	static_assert(std::is_same_v< ForwardT<const TrivialRelocat &&>, const TrivialRelocat && >);
	static_assert(std::is_same_v< ForwardT<TrivialRelocat &>, TrivialRelocat & >);
	static_assert(std::is_same_v< ForwardT<const TrivialRelocat &>, const TrivialRelocat & >);

#ifdef _MSC_VER
	static_assert(std::is_same_v< ForwardT<P>, P >);
	static_assert(std::is_same_v< ForwardT<P &&>, P >);

	static_assert(std::is_same_v< ForwardT<TrivialRelocat>, TrivialRelocat >);
	static_assert(std::is_same_v< ForwardT<TrivialRelocat &&>, TrivialRelocat >);
#else
	static_assert(std::is_same_v< ForwardT<P>, P && >);
	static_assert(std::is_same_v< ForwardT<P &&>, P && >);

	static_assert(std::is_same_v< ForwardT<TrivialRelocat>, TrivialRelocat && >);
	static_assert(std::is_same_v< ForwardT<TrivialRelocat &&>, TrivialRelocat && >);
#endif
	{
		using A = std::array<std::size_t, 3>;
		static_assert(std::is_same_v< ForwardT<A>, A && >);
		static_assert(std::is_same_v< ForwardT<A &&>, A && >);
		static_assert(std::is_same_v< ForwardT<A const>, const A && >);
		static_assert(std::is_same_v< ForwardT<const A &&>, const A && >);
		static_assert(std::is_same_v< ForwardT<A &>, A & >);
		static_assert(std::is_same_v< ForwardT<const A &>, const A & >);
	}
	using A = std::array<double, 1>;
	static_assert(std::is_same_v< ForwardT<A const>, A >);
	static_assert(std::is_same_v< ForwardT<A &&>, A >);
	static_assert(std::is_same_v< ForwardT<const A &&>, A >);
	static_assert(std::is_same_v< ForwardT<const A &>, A >);
	static_assert(std::is_same_v< ForwardT<A &>, A & >);

#if HAS_STD_PMR
	using Alloc = std::pmr::polymorphic_allocator<int>;
	static_assert(std::is_same_v< ForwardT<Alloc>, Alloc >);
	static_assert(std::is_same_v< ForwardT<Alloc &&>, Alloc >);
	static_assert(std::is_same_v< ForwardT<const Alloc &>, Alloc >);
#endif
}

template<typename SizeT>
struct DummyRange
{
	SizeT n;

	SizeT size() const { return n; }
};

TEST(utilTest, ssize)
{
	static_assert(oel::range_is_sized< DummyRange<int> >);

	using test  = decltype( oel::ssize(DummyRange<unsigned short>{0}) );
	using test2 = decltype( oel::ssize(DummyRange<std::uintmax_t>{0}) );

	static_assert(std::is_same<test, std::ptrdiff_t>::value);
	static_assert(std::is_same<test2, std::intmax_t>::value);
}

TEST(utilTest, indexValid)
{
	using namespace oel;

	using BigInt  = long long;
	using BigUint = unsigned long long;

	DummyRange<int> r1{1};

	EXPECT_TRUE(index_valid(r1, 0));
	EXPECT_FALSE(index_valid(r1, BigUint{1}));
	EXPECT_FALSE(index_valid(r1, BigInt{-1}));
	{
		constexpr auto size = std::numeric_limits<unsigned>::max();
		DummyRange<unsigned> r2{size};

		EXPECT_FALSE(index_valid(r2, BigInt{size}));
		EXPECT_TRUE(index_valid(r2, BigUint{size - 1}));
	}
	{
		constexpr auto size = std::numeric_limits<BigInt>::max();
		DummyRange<BigUint> r2{as_unsigned(size)};

		EXPECT_FALSE(index_valid(r2, size));
		EXPECT_TRUE(index_valid(r2, size - 1));
	}
}

TEST(utilTest, memberFn)
{
	struct Moo
	{
		int noParam() const & { return -2; }

		const int & twoParam(int & y, const int && z) && { return y += z; }
	};

	auto f = OEL_MEMBER_FN(noParam);
	auto g = OEL_MEMBER_FN(twoParam);

	Moo const x{};
	EXPECT_EQ(-2, f(x));

	auto y = 1;
	EXPECT_EQ(3, g(Moo{}, y, 2));
	using T = decltype( g(Moo{}, y, 2) );
	static_assert(std::is_same_v<T, const int &>);
}

TEST(utilTest, memberVar)
{
	struct Baz
	{
		int val;
	} x{7};

	auto f = OEL_MEMBER_VAR(val);

	static_assert(std::is_same_v<decltype( f(x) ), int &>);

	using T = decltype( f(static_cast<const Baz &&>(x)) );
	static_assert(std::is_same_v<T, const int &&>);

	EXPECT_EQ(7, f(x));
}

TEST(utilTest, toPointerContiguous)
{
	using oel::iter::as_contiguous_address;
	{
		std::basic_string<wchar_t> s;
		using P  = decltype( as_contiguous_address(s.begin()) );
		using CP = decltype( as_contiguous_address(s.cbegin()) );
		static_assert(std::is_same<P, wchar_t *>::value);
		static_assert(std::is_same<CP, const wchar_t *>::value);

		std::string_view v;
		using Q = decltype( as_contiguous_address(v.begin()) );
		static_assert(std::is_same<Q, const char *>::value);
	}
	std::array<int, 3> a;

	using P  = decltype(as_contiguous_address( std::make_move_iterator(a.begin()) ));
	using CP = decltype( as_contiguous_address(a.cbegin()) );
	static_assert(std::is_same<P, int *>::value);
	static_assert(std::is_same<CP, const int *>::value);

#if __cpp_lib_concepts
	auto addr = &a[1];
	oel::iter::_dynarrayChecked<const int *> it{{}, addr, nullptr, 0};
	auto result = std::to_address(it);
	static_assert(std::is_same<const int *, decltype(result)>());
	EXPECT_EQ(addr, result);
#endif
}

struct EmptyRandomAccessRange {};

int * begin(EmptyRandomAccessRange) { return {}; }
int * end(EmptyRandomAccessRange)   { return {}; }

TEST(utilTest, detailSize_rangeNoMember)
{
	EmptyRandomAccessRange r;
	static_assert(oel::range_is_sized<decltype(r)>);
	EXPECT_EQ(0, oel::_detail::Size(r));
}

template< typename I >
struct TooSimpleIter
{
	using value_type = double;
	using reference = double &;
	using pointer = void;
	using difference_type = std::ptrdiff_t;
	using iterator_category = std::random_access_iterator_tag;

	I it;

	void operator++()
	{
		++it;
	}

	bool operator==(I right) const
	{
		return it == right;
	}
	bool operator!=(I right) const
	{
		return it != right;
	}

	friend std::ptrdiff_t operator -(I, TooSimpleIter)
	{
		static_assert(sizeof(I) == -1, "Not supposed to be here");
		return {};
	}
};

#if !defined _MSC_VER or _MSC_VER >= 1920
template< typename I >
constexpr bool oel::disable_sized_sentinel_for< I, TooSimpleIter<I> > = true;
#else
namespace oel
{
template< typename I >
constexpr bool disable_sized_sentinel_for< I, TooSimpleIter<I> > = true;
}
#endif

TEST(utilTest, detailCountOrEnd_disabledSize)
{
	using A = std::array<double, 1>;
	A src{1};
	auto v = oel::view::subrange(TooSimpleIter<A::iterator>{src.begin()}, src.end());

	static_assert( !oel::range_is_sized<decltype(v)> );
	static_assert(oel::_detail::rangeIsForwardOrSized<decltype(v)>);

	auto n = oel::_detail::UDist(v);
	EXPECT_EQ(1u, n);
}
