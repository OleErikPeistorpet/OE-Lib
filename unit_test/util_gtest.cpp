// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "test_classes.h"
#include "make_unique.h"
#include "dynarray.h"

#include "gtest/gtest.h"
#include <list>
#include <set>
#include <array>


namespace
{
	static_assert(oel::is_trivially_relocatable< std::pair<int *, std::unique_ptr<int>> >(), "?");
	static_assert(oel::is_trivially_relocatable< std::tuple<std::unique_ptr<double>> >(), "?");
	static_assert(oel::is_trivially_relocatable< std::tuple<> >::value, "?");

	struct NonTrivialAssign
	{
		void operator =(const NonTrivialAssign &) { ; }
	};
	struct NonTrivialDestruct
	{
		~NonTrivialDestruct() { ; }
	};
	static_assert( !std::is_trivially_copyable<NonTrivialAssign>::value, "?" );
	static_assert( !std::is_trivially_copyable<NonTrivialDestruct>::value, "?" );

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

	static_assert(!oel::can_memmove_with< int *, float * >::value, "?");
	static_assert(!oel::can_memmove_with< int *, std::set<int>::iterator >(), "?");
	static_assert(!oel::can_memmove_with< int *, std::move_iterator<std::list<int>::iterator> >(), "?");
	static_assert(oel::can_memmove_with< std::array<int, 1>::iterator, std::move_iterator<int *> >(), "?");
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

	using BigInt =
	#if LONG_MAX > INT_MAX
		long;
	#else
		long long;
	#endif
	using BigUint = std::make_unsigned_t<BigInt>;

	DummyRange<unsigned> r1{1};

	EXPECT_TRUE(index_valid(r1, (std::ptrdiff_t) 0));
	EXPECT_FALSE(index_valid(r1, (size_t) 1));
	EXPECT_FALSE(index_valid(r1, -1));
	EXPECT_FALSE(index_valid(r1, (size_t) -1));
	{
		constexpr auto size = std::numeric_limits<unsigned>::max();
		DummyRange<unsigned> r2{size};

		EXPECT_FALSE(index_valid(r2, (BigInt) -2));
		EXPECT_FALSE(index_valid(r2, size));
		EXPECT_TRUE(index_valid(r2, size - 1));
		EXPECT_TRUE(index_valid(r2, 0));
	}
	{
		constexpr auto size = as_unsigned(std::numeric_limits<BigInt>::max());
		DummyRange<BigUint> r2{size};

		EXPECT_FALSE(index_valid(r2, (BigUint) -2));
		EXPECT_FALSE(index_valid(r2, (BigInt) -2));
		EXPECT_TRUE(index_valid(r2, size - 1));
	}
}

struct OneSizeT
{
	size_t val;
};

TEST(utilTest, makeUnique)
{
	auto ps = oel::make_unique_for_overwrite<std::string[]>(2);
	EXPECT_TRUE(ps[0].empty());
	EXPECT_TRUE(ps[1].empty());

	{
		auto p = oel::make_unique<OneSizeT>(7U);
		EXPECT_EQ(7U, p->val);

		auto a = oel::make_unique<OneSizeT[]>(5);
		for (size_t i = 0; i < 5; ++i)
			EXPECT_EQ(0U, a[i].val);
	}
	auto p2 = oel::make_unique<std::list<int>>(4, 6);
	EXPECT_EQ(4U, p2->size());
	EXPECT_EQ(6, p2->front());
	EXPECT_EQ(6, p2->back());
}

template<typename T>
struct PointerLike
{
	using difference_type = ptrdiff_t;
	using element_type = T;

	T * p;

	explicit operator T *() const { return p; }

	T * operator->() const { return p; }
	T & operator *() const { return *p; }
};
static_assert(std::is_trivially_default_constructible< PointerLike<bool> >::value, "?");
static_assert(std::is_trivially_copyable< PointerLike<bool> >::value, "?");

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

	auto addr = &a[0];
	dynarray_iterator<PointerLike<int>, int> it{{addr}, {nullptr}, 0};
	auto result = to_pointer_contiguous(it);
	static_assert(std::is_same<int *, decltype(result)>(), "?");
	EXPECT_EQ(addr, result);
}
