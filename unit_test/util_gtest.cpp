// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "test_classes.h"
#include "make_unique.h"
#include "dynarray.h"

#include "gtest/gtest.h"
#include <list>
#include <set>
#include <array>
#if HAS_STD_PMR
#include <memory_resource>
#endif

namespace
{
	static_assert(oel::is_trivially_relocatable< std::pair<int *, std::unique_ptr<int>> >(), "?");
	static_assert(oel::is_trivially_relocatable< std::tuple<std::unique_ptr<double>> >(), "?");
	static_assert(oel::is_trivially_relocatable< std::tuple<> >::value, "?");

	struct NonTrivialDestruct
	{
		~NonTrivialDestruct() { ; }
	};

	static_assert( !oel::is_trivially_relocatable< std::tuple<int, NonTrivialDestruct, int> >(), "?" );

#if (defined _CPPLIB_VER or defined _LIBCPP_VERSION or defined __GLIBCXX__) and !_GLIBCXX_USE_CXX11_ABI
	static_assert(oel::is_trivially_relocatable< std::string >::value, "?");
#endif
#ifndef OEL_NO_BOOST
	static_assert(oel::is_trivially_relocatable< boost::circular_buffer<int, oel::allocator<int>> >(), "?");
#endif

	struct alignas(32) Foo { int a[24]; };
	static_assert(alignof(oel::storage_for<Foo>) == 32, "?");
	static_assert(sizeof(oel::storage_for<Foo>) == sizeof(Foo), "?");

	using ListI = std::list<int>::iterator;
	static_assert(!oel::can_memmove_with< int *, float * >::value, "?");
	static_assert(!oel::can_memmove_with< int *, std::set<int>::iterator >(), "?");
	static_assert(!oel::can_memmove_with< int *, std::move_iterator<ListI> >(), "?");
	static_assert(oel::can_memmove_with< std::array<int, 1>::iterator, std::move_iterator<const int *> >(), "?");

	static_assert(!oel::iter_is_random_access<ListI>, "?");
}

TEST(utilTest, ForwardT)
{
	using oel::_detail::ForwardT;

	static_assert(std::is_same< ForwardT<double const>, double >::value, "?");
	static_assert(std::is_same< ForwardT<double &&>, double >::value, "?");
	static_assert(std::is_same< ForwardT<const double &&>, double >::value, "?");
	static_assert(std::is_same< ForwardT<const double &>, double >::value, "?");
	static_assert(std::is_same< ForwardT<double &>, double & >::value, "?");

	static_assert(std::is_same< ForwardT<int[1]>, int(&&)[1] >::value, "?");
	static_assert(std::is_same< ForwardT<int(&&)[1]>, int(&&)[1] >::value, "?");
	static_assert(std::is_same< ForwardT<const int(&)[1]>, const int(&)[1] >::value, "?");

	using P = std::unique_ptr<int>;
	static_assert(std::is_same< ForwardT<P const>, const P && >::value, "?");
	static_assert(std::is_same< ForwardT<const P &&>, const P && >::value, "?");
	static_assert(std::is_same< ForwardT<P &>, P & >::value, "?");
	static_assert(std::is_same< ForwardT<const P &>, const P & >::value, "?");

	// Small, non-trivial copy
	static_assert(std::is_same< ForwardT<TrivialRelocat const>, const TrivialRelocat && >(), "?");
	static_assert(std::is_same< ForwardT<const TrivialRelocat &&>, const TrivialRelocat && >(), "?");
	static_assert(std::is_same< ForwardT<TrivialRelocat &>, TrivialRelocat & >(), "?");
	static_assert(std::is_same< ForwardT<const TrivialRelocat &>, const TrivialRelocat & >(), "?");

#ifdef _MSC_VER
	static_assert(std::is_same< ForwardT<P>, P >::value, "?");
	static_assert(std::is_same< ForwardT<P &&>, P >::value, "?");

	static_assert(std::is_same< ForwardT<TrivialRelocat>, TrivialRelocat >::value, "?");
	static_assert(std::is_same< ForwardT<TrivialRelocat &&>, TrivialRelocat >::value, "?");
#else
	static_assert(std::is_same< ForwardT<P>, P && >::value, "?");
	static_assert(std::is_same< ForwardT<P &&>, P && >::value, "?");

	static_assert(std::is_same< ForwardT<TrivialRelocat>, TrivialRelocat && >::value, "?");
	static_assert(std::is_same< ForwardT<TrivialRelocat &&>, TrivialRelocat && >::value, "?");
#endif

#ifdef _MSC_VER
	using A = std::array<double, 2>;
	static_assert(std::is_same< ForwardT<A>, A && >::value, "?");
	static_assert(std::is_same< ForwardT<A &&>, A && >::value, "?");
	static_assert(std::is_same< ForwardT<A const>, const A && >::value, "?");
	static_assert(std::is_same< ForwardT<const A &&>, const A && >::value, "?");
	static_assert(std::is_same< ForwardT<A &>, A & >::value, "?");
	static_assert(std::is_same< ForwardT<const A &>, const A & >::value, "?");
#else
	using A = std::array<int *, 2>;
	static_assert(std::is_same< ForwardT<A const>, A >::value, "?");
	static_assert(std::is_same< ForwardT<A &&>, A >::value, "?");
	static_assert(std::is_same< ForwardT<const A &&>, A >::value, "?");
	static_assert(std::is_same< ForwardT<const A &>, A >::value, "?");
	static_assert(std::is_same< ForwardT<A &>, A & >::value, "?");
#endif

#if HAS_STD_PMR
	using Alloc = std::pmr::polymorphic_allocator<int>;
	static_assert(std::is_same< ForwardT<Alloc>, Alloc >::value, "?");
	static_assert(std::is_same< ForwardT<Alloc &&>, Alloc >::value, "?");
	static_assert(std::is_same< ForwardT<const Alloc &>, Alloc >::value, "?");
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
	using test  = decltype( oel::ssize(DummyRange<unsigned short>{0}) );
	using test2 = decltype( oel::ssize(DummyRange<std::uintmax_t>{0}) );

	static_assert(std::is_same<test, std::ptrdiff_t>::value, "?");
	static_assert(std::is_same<test2, std::intmax_t>::value, "?");
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

TEST(utilTest, makeUnique)
{
	auto ps = oel::make_unique_for_overwrite<std::string[]>(2);
	EXPECT_TRUE(ps[0].empty());
	EXPECT_TRUE(ps[1].empty());

	auto a = oel::make_unique_for_overwrite<int[]>(5);
	for (int i = 0; i < 5; ++i)
		a[i] = i;
}

TEST(utilTest, toPointerContiguous)
{
	using namespace oel;
	{
		std::basic_string<wchar_t> s;
		using P  = decltype( to_pointer_contiguous(s.begin()) );
		using CP = decltype( to_pointer_contiguous(s.cbegin()) );
		static_assert(std::is_same<P, wchar_t *>::value, "?");
		static_assert(std::is_same<CP, const wchar_t *>::value, "?");

	#if _HAS_CXX17
		std::string_view v;
		using Q = decltype( to_pointer_contiguous(v.begin()) );
		static_assert(std::is_same<Q, const char *>::value, "?");
	#endif
	}
	std::array<int, 3> a;

	using P  = decltype(to_pointer_contiguous( std::make_move_iterator(a.begin()) ));
	using CP = decltype( to_pointer_contiguous(a.cbegin()) );
	static_assert(std::is_same<P, int *>::value, "?");
	static_assert(std::is_same<CP, const int *>::value, "?");

#if __cpp_lib_concepts
	auto addr = &a[1];
	dynarray_iterator<const int *> it{addr, nullptr, 0};
	auto result = std::to_address(it);
	static_assert(std::is_same<const int *, decltype(result)>(), "?");
	EXPECT_EQ(addr, result);
#endif
}

struct EmptyRandomAccessRange {};

int * begin(EmptyRandomAccessRange) { return {}; }
int * end(EmptyRandomAccessRange)   { return {}; }

TEST(utilTest, detailSize_rangeNoMember)
{
	EmptyRandomAccessRange r;
	EXPECT_EQ(0, oel::_detail::Size(r));
}
