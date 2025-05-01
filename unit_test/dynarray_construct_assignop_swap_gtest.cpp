// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "test_classes.h"
#include "mem_leak_detector.h"
#include "dynarray.h"

#include <array>
#include <string>
#include <forward_list>
#include <sstream>

int MyCounter::nConstructions;
int MyCounter::nDestruct;
int MyCounter::countToThrowOn = -1;

TrackingAllocData g_allocCount;

using namespace oel;

class dynarrayConstructTest : public ::testing::Test
{
protected:
	std::array<unsigned, 3> sizes{0, 1, 200};

	~dynarrayConstructTest()
	{
		EXPECT_EQ(g_allocCount.nAllocations, g_allocCount.nDeallocations);
		EXPECT_EQ(MyCounter::nConstructions, MyCounter::nDestruct);

		g_allocCount.clear();
		MyCounter::clearCount();
	}

	template<typename T>
	void testFillTrivial(T val)
	{
		dynarrayTrackingAlloc<T> a(11, val);

		ASSERT_EQ(a.size(), 11U);
		for (const auto & e : a)
			EXPECT_EQ(val, e);

		ASSERT_EQ(g_allocCount.nDeallocations + 1, g_allocCount.nAllocations);
	}
};

namespace
{
template< typename T = int >
struct NonConstexprAlloc : oel::allocator<T>
{
	NonConstexprAlloc() {}
	NonConstexprAlloc(const NonConstexprAlloc &) : allocator<T>{} {}
};
}

#if __cpp_constinit
void testConstInitCompile()
{
	constinit static dynarray<int> d;
}
#endif

void testNonConstexprCompile()
{
	static dynarray<int, NonConstexprAlloc<>> d;
	[[maybe_unused]] auto d2 = dynarray<int, NonConstexprAlloc<>>(NonConstexprAlloc<>{});
}

TEST_F(dynarrayConstructTest, constructEmpty)
{
	dynarrayTrackingAlloc<TrivialDefaultConstruct> a;

	ASSERT_TRUE(a.get_allocator() == TrackingAllocator<TrivialDefaultConstruct>{});
	EXPECT_EQ(0U, a.capacity());
	EXPECT_EQ(0, g_allocCount.nAllocations);
	ASSERT_EQ(0, g_allocCount.nDeallocations);
}

#if defined _CPPUNWIND or defined __EXCEPTIONS
TEST_F(dynarrayConstructTest, greaterThanMax)
{
	struct Size2
	{
		char a[2];
	};

	using Dynarr = dynarray<Size2>;
#ifndef _MSC_VER
	const // unreachable code warnings
#endif
		size_t n = std::numeric_limits<size_t>::max() / 2 + 1;

	EXPECT_THROW(Dynarr d(reserve, n), std::length_error);
	EXPECT_THROW(Dynarr d(n, for_overwrite), std::length_error);
	EXPECT_THROW(Dynarr d(n), std::length_error);
	EXPECT_THROW(Dynarr d(n, Size2{{}}), std::length_error);
}
#endif

TEST_F(dynarrayConstructTest, constructReserve)
{
	for (auto const n : sizes)
	{
		auto const nExpectAlloc = g_allocCount.nAllocations + 1;

		dynarrayTrackingAlloc<TrivialDefaultConstruct> a(reserve, n);

		ASSERT_TRUE(a.empty());
		ASSERT_TRUE(a.capacity() >= n);

		if (n > 0)
		{	ASSERT_EQ(nExpectAlloc, g_allocCount.nAllocations); }
	}
	ASSERT_EQ(0, g_allocCount.nConstructCalls);
}

TEST_F(dynarrayConstructTest, constructForOverwriteTrivial)
{
	for (auto const n : sizes)
	{
		auto const nExpectAlloc = g_allocCount.nAllocations + 1;

		dynarrayTrackingAlloc<TrivialDefaultConstruct> a(n, for_overwrite);

		ASSERT_EQ(a.size(), n);

		if (n > 0)
		{	ASSERT_EQ(nExpectAlloc, g_allocCount.nAllocations); }
	}
	EXPECT_EQ(0, g_allocCount.nConstructCalls);
}

TEST_F(dynarrayConstructTest, constructForOverwrite)
{
	for (auto const n : sizes)
	{
		g_allocCount.nConstructCalls = 0;
		NontrivialConstruct::clearCount();

		auto const nExpectAlloc = g_allocCount.nAllocations + 1;
		{
			dynarrayTrackingAlloc<NontrivialConstruct> a(n, for_overwrite);

			ASSERT_EQ(as_signed(n), g_allocCount.nConstructCalls);
			ASSERT_EQ(as_signed(n), NontrivialConstruct::nConstructions);

			ASSERT_EQ(a.size(), n);

			if (n > 0)
			{	ASSERT_EQ(nExpectAlloc, g_allocCount.nAllocations); }
		}
		ASSERT_EQ(NontrivialConstruct::nConstructions, NontrivialConstruct::nDestruct);
	}
}

TEST_F(dynarrayConstructTest, constructNTrivial)
{
	for (auto const n : sizes)
	{
		g_allocCount.nConstructCalls = 0;

		auto const nExpectAlloc = g_allocCount.nAllocations + 1;

		dynarrayTrackingAlloc<TrivialDefaultConstruct> a(n);

		EXPECT_EQ(0, g_allocCount.nConstructCalls);

		ASSERT_EQ(a.size(), n);

		auto const asBytes = reinterpret_cast<const char *>(a.data());
		auto const nBytes = a.size() * sizeof a[0];
		for (size_t i{}; i != nBytes; ++i)
			EXPECT_TRUE(asBytes[i] == 0);

		if (n > 0)
		{	ASSERT_EQ(nExpectAlloc, g_allocCount.nAllocations); }
	}
}

TEST_F(dynarrayConstructTest, constructN)
{
	for (auto const n : sizes)
	{
		g_allocCount.nConstructCalls = 0;

		auto const nExpectAlloc = g_allocCount.nAllocations + 1;

		dynarrayTrackingAlloc<NontrivialConstruct> a(n);

		ASSERT_EQ(as_signed(n), g_allocCount.nConstructCalls);

		ASSERT_EQ(a.size(), n);

		if (n > 0)
		{	ASSERT_EQ(nExpectAlloc, g_allocCount.nAllocations); }
	}
}

TEST_F(dynarrayConstructTest, constructNFillTrivial)
{
	testFillTrivial(true);
	testFillTrivial<char>(97);
	testFillTrivial<int>(97);
	testFillTrivial(std::byte{97});
}

TEST_F(dynarrayConstructTest, constructNFill)
{
	{
		dynarrayTrackingAlloc<TrivialRelocat> a(11, TrivialRelocat(97));

		ASSERT_EQ(a.size(), 11U);
		for (const auto & e : a)
			ASSERT_TRUE(97.0 == *e);

		EXPECT_EQ(1 + 11, TrivialRelocat::nConstructions);

		ASSERT_EQ(g_allocCount.nDeallocations + 1, g_allocCount.nAllocations);
	}
}


TEST_F(dynarrayConstructTest, constructInitList)
{
	{
		auto il = {1.2, 3.4, 5.6, 7.8};
		dynarrayTrackingAlloc<double> a(il);

		ASSERT_TRUE(std::equal( a.begin(), a.end(), begin(il) ));
		ASSERT_EQ(4U, a.size());

		ASSERT_EQ(1, g_allocCount.nAllocations);
	}
	{
		dynarrayTrackingAlloc<TrivialRelocat> a(std::initializer_list<TrivialRelocat>{});

		ASSERT_TRUE(a.empty());
		EXPECT_EQ(0, TrivialRelocat::nConstructions);
	}
}

TEST_F(dynarrayConstructTest, deductionGuides)
{
	using Base = std::array<int, 2>;
	struct NonMoveableRange : public Base
	{
		NonMoveableRange() : Base{} {}

		NonMoveableRange(const NonMoveableRange &) = delete;
		NonMoveableRange(NonMoveableRange &&) = delete;
	}
	ar;

	auto d = ar | to_dynarray(StatefulAllocator<int>{});
	static_assert(std::is_same< decltype(d)::allocator_type, StatefulAllocator<int> >());
	EXPECT_EQ(d.size(), ar.size());

	dynarray fromTemp(from_range, std::array<int, 1>{});
	static_assert(std::is_same< decltype(fromTemp)::allocator_type, oel::allocator<int> >());

	dynarray sizeAndVal(2, 1.f);
	static_assert(std::is_same<decltype(sizeAndVal)::value_type, float>());
	EXPECT_TRUE(sizeAndVal.at(1) == 1.f);
}

TEST_F(dynarrayConstructTest, constructContiguousRange)
{
	std::string str = "AbCd";
	dynarray test(from_range, str);
	static_assert(std::is_same<decltype(test)::value_type, char>());
	EXPECT_TRUE( 0 == str.compare(0, 4, test.data(), test.size()) );
}

TEST_F(dynarrayConstructTest, constructRangeNoCopyAssign)
{
	auto il = { 1.2, 3.4 };
	dynarray<MoveOnly> test(from_range, il);
	EXPECT_TRUE(test.size() == 2);
}

TEST_F(dynarrayConstructTest, constructForwardRangeNoSize)
{
	for (auto const n : {0u, 1u, 59u})
	{
		std::forward_list<int> li(n, -6);
		dynarray<int> d(from_range, li);
		EXPECT_EQ(n, d.size());
		if (0 != n)
		{
			EXPECT_EQ(-6, d.front());
			EXPECT_EQ(-6, d.back());
		}
	}
}

TEST_F(dynarrayConstructTest, constructRangeMutableBeginSize)
{
	int src[1] {1};
	dynarray<int> d(from_range, ToMutableBeginSizeView(src));
	EXPECT_EQ(1u, d.size());
}

#ifndef NO_VIEWS_ISTREAM

TEST_F(dynarrayConstructTest, constructMoveOnlyIterator)
{
	std::istringstream words{"Falling Anywhere"};
	auto d = dynarray(from_range, std::views::istream<std::string>(words));
	EXPECT_EQ(2u, d.size());
	EXPECT_EQ("Falling", d[0]);
	EXPECT_EQ("Anywhere", d[1]);
}
#endif

TEST_F(dynarrayConstructTest, copyConstruct)
{
	using Al = StatefulAllocator<TrivialRelocat>;
	auto x = dynarray< TrivialRelocat, Al >({TrivialRelocat{0.5}}, Al(-5));

	auto y = dynarray(x);
	EXPECT_EQ(-5, y.get_allocator().id);
	EXPECT_EQ(2, g_allocCount.nAllocations);

	auto const z = dynarray(y, Al(7));
	EXPECT_EQ(0.5, *z.front());

	auto const d = dynarray(std::move(z));
	EXPECT_EQ(0.5, *d.front());

	auto const e = dynarray(std::move(d), Al(9));
	EXPECT_EQ(9, e.get_allocator().id);
	EXPECT_EQ(5, g_allocCount.nAllocations);
}

template<typename Alloc>
void testMoveConstruct(Alloc a0, Alloc a1)
{
	for (auto const nr : {0, 2})
	{
		dynarray<MoveOnly, Alloc> right(a0);

		for (int i = 0; i < nr; ++i)
			right.emplace_back(0.5);

		auto const nAllocBefore = g_allocCount.nAllocations;

		auto const ptr = right.data();

		dynarray<MoveOnly, Alloc> left(std::move(right), a1);

		EXPECT_TRUE(left.get_allocator() == a1);

		EXPECT_EQ(nAllocBefore, g_allocCount.nAllocations);
		EXPECT_EQ(nr, MoveOnly::nConstructions - MoveOnly::nDestruct);

		EXPECT_TRUE(right.empty());
		EXPECT_EQ(0U, right.capacity());
		ASSERT_EQ(nr, ssize(left));
		EXPECT_EQ(ptr, left.data());
	}
	EXPECT_EQ(g_allocCount.nAllocations, g_allocCount.nDeallocations);
}

TEST_F(dynarrayConstructTest, moveConstructWithAlloc)
{
	TrackingAllocator<MoveOnly> a;
	testMoveConstruct(a, a);
}

TEST_F(dynarrayConstructTest, moveConstructWithStatefulAlloc)
{
	using Al = StatefulAllocator<MoveOnly, false>;
	testMoveConstruct(Al(0), Al(0));
}

struct NonAssignable
{
	NonAssignable(NonAssignable &&) = default;
	void operator =(NonAssignable &&) = delete;
};

TEST_F(dynarrayConstructTest, moveConstructNonAssignable)
{	// just to check that it compiles
	dynarray<NonAssignable> d(dynarray<NonAssignable>{}, allocator<>{});
}

template<typename T>
void testConstructMoveElements()
{
	g_allocCount.clear();
	T::clearCount();
	// not propagating, not equal, cannot steal the memory
	for (auto const na : {0u, 1u, 101u})
	{
		using Alloc = StatefulAllocator<T, false, false>;
		dynarray<T, Alloc> a(reserve, na, Alloc{1});

		for (unsigned i{}; i < na; ++i)
			a.emplace_back(i + 0.5);

		auto const capBefore = a.capacity();

		auto const nExpectAlloc = g_allocCount.nAllocations + (a.empty() ? 0 : 1);

		dynarray<T, Alloc> b(std::move(a), Alloc{2});

		ASSERT_FALSE(a.get_allocator() == b.get_allocator());

		EXPECT_EQ(1, a.get_allocator().id);
		EXPECT_EQ(2, b.get_allocator().id);

		EXPECT_EQ(nExpectAlloc, g_allocCount.nAllocations);

		EXPECT_EQ(ssize(a) + ssize(b), T::nConstructions - T::nDestruct);

		EXPECT_EQ(capBefore, a.capacity());
		ASSERT_EQ(na, b.size());
		for (unsigned i{}; i < na; ++i)
			EXPECT_TRUE(b[i].hasValue() and *b[i] == i + 0.5);
	}
	EXPECT_EQ(T::nConstructions, T::nDestruct);
	EXPECT_EQ(g_allocCount.nAllocations, g_allocCount.nDeallocations);
}

TEST_F(dynarrayConstructTest, moveConstructUnequalAlloc)
{
	testConstructMoveElements<MoveOnly>();
	testConstructMoveElements<TrivialRelocat>();
}

template<typename Alloc>
void testMoveAssign(Alloc a0, Alloc a1)
{
	for (auto const nl : {0, 1, 80})
	{
		dynarray<MoveOnly, Alloc> right(a0);
		dynarray<MoveOnly, Alloc> left(a1);

		for (int i = 0; i < 9; ++i)
			right.emplace_back(0.5);

		for (int i = 0; i < nl; ++i)
			left.emplace_back(0.5);

		auto const nAllocBefore = g_allocCount.nAllocations;

		auto const ptr = right.data();

		left = std::move(right);

		if (std::allocator_traits<Alloc>::propagate_on_container_move_assignment::value)
			EXPECT_TRUE(left.get_allocator() == a0);
		else
			EXPECT_TRUE(left.get_allocator() == a1);

		EXPECT_EQ(nAllocBefore, g_allocCount.nAllocations);
		EXPECT_EQ(9, MoveOnly::nConstructions - MoveOnly::nDestruct);

		if (0 == nl)
		{
			ASSERT_TRUE(right.empty());
			EXPECT_EQ(0U, right.capacity());
		}
		ASSERT_EQ(9U, left.size());
		EXPECT_EQ(ptr, left.data());
	}
}

TEST_F(dynarrayConstructTest, moveAssign)
{
	TrackingAllocator<MoveOnly> a;
	testMoveAssign(a, a);
}

TEST_F(dynarrayConstructTest, moveAssignStatefulAlloc)
{
	using PropagateAlloc = StatefulAllocator<MoveOnly, true>;
	static_assert(std::is_nothrow_move_assignable< dynarray<MoveOnly, PropagateAlloc> >::value);
	testMoveAssign(PropagateAlloc(0), PropagateAlloc(1));
}

template<typename T>
void testAssignMoveElements()
{
	// not propagating, not equal, cannot steal the memory
	for (auto const na : {0, 1, 101})
	{
		for (auto const nb : {0, 1, 2})
		{
			using Alloc = StatefulAllocator<T, false, false>;
			dynarray<T, Alloc> a(reserve, as_unsigned(na), Alloc{1});
			dynarray<T, Alloc> b(reserve, as_unsigned(nb), Alloc{2});

			ASSERT_FALSE(a.get_allocator() == b.get_allocator());

			for (int i = 0; i < na; ++i)
				a.emplace_back(i + 0.5);

			for (int i = 0; i < nb; ++i)
				b.emplace_back(i + 0.5);

			auto const capBefore = a.capacity();

			auto const nExpectAlloc = g_allocCount.nAllocations + (b.size() < a.size() ? 1 : 0);

			b = std::move(a);

			EXPECT_EQ(1, a.get_allocator().id);
			EXPECT_EQ(2, b.get_allocator().id);

			EXPECT_EQ(nExpectAlloc, g_allocCount.nAllocations);

			EXPECT_EQ(ssize(a) + ssize(b), T::nConstructions - T::nDestruct);

			EXPECT_EQ(capBefore, a.capacity());
			ASSERT_EQ(na, ssize(b));
			for (int i = 0; i < na; ++i)
				EXPECT_TRUE(b[i].hasValue() and *b[i] == i + 0.5);
		}
		EXPECT_EQ(T::nConstructions, T::nDestruct);
		EXPECT_EQ(g_allocCount.nAllocations, g_allocCount.nDeallocations);
	}
}

TEST_F(dynarrayConstructTest, moveAssignNoPropagateEqualAlloc)
{
	using Alloc = StatefulAllocator<MoveOnly, false>;
	testMoveAssign(Alloc{}, Alloc{});
}

TEST_F(dynarrayConstructTest, moveAssignNoPropagateAlloc)
{
	testAssignMoveElements<MoveOnly>();
}

TEST_F(dynarrayConstructTest, moveAssignNoPropagateAllocTrivialReloc)
{
	testAssignMoveElements<TrivialRelocat>();
}

#if HAS_STD_PMR

#include <memory_resource>

template< typename T >
using PmrDynarray = dynarray< T, std::pmr::polymorphic_allocator<T> >;

TEST_F(dynarrayConstructTest, moveAssignPolymorphicAlloc)
{
	static_assert(!std::is_nothrow_move_assignable_v< PmrDynarray<int> >);

	using Nested = PmrDynarray< PmrDynarray<int> >;
	std::pmr::monotonic_buffer_resource bufRes{};
	auto a = Nested(1);
	auto b = Nested(1, &bufRes);
	ASSERT_TRUE(b.get_allocator() == b.front().get_allocator());
	ASSERT_TRUE(a.front().get_allocator() != b.front().get_allocator());

	b = std::move(a);

	EXPECT_EQ(b.get_allocator().resource(), &bufRes);
	EXPECT_EQ(b.front().get_allocator().resource(), &bufRes);
	EXPECT_TRUE(a.get_allocator() != b.get_allocator());
	EXPECT_TRUE(b.get_allocator() == b.front().get_allocator());
}
#endif

TEST_F(dynarrayConstructTest, selfMoveAssign)
{
	dynarray<int> d(3, -3);
	{
		auto tmp = std::move(d);
		d = std::move(d);
		d = std::move(tmp);
	}
	EXPECT_EQ(3U, d.size());
	EXPECT_EQ(-3, d.back());
}

TEST_F(dynarrayConstructTest, selfCopyAssign)
{
	{
		dynarrayTrackingAlloc<int> d;
		d = d;
		EXPECT_TRUE(d.empty());
		EXPECT_TRUE(d.capacity() == 0);
	}
	{
		auto il = {1, 2, 3, 4};
		dynarray<int> d(il);
		d = d;
		EXPECT_EQ(d.size(), il.size());
		EXPECT_TRUE(std::equal( d.begin(), d.end(), begin(il) ));
	}
	{
		dynarrayTrackingAlloc<TrivialRelocat> nt;
		nt = {TrivialRelocat(5)};
		nt = nt;
		EXPECT_EQ(5, *nt[0]);
	}
}

#if OEL_HAS_EXCEPTIONS

TEST_F(dynarrayConstructTest, constructInputRangeThrowing)
{
	std::stringstream ss("1 2");
	std::istream_iterator<double> f{ss}, l{};
	MoveOnly::countToThrowOn = 1;

	ASSERT_THROW(
		dynarray<MoveOnly>(from_range, view::subrange(f, l)),
		TestException );
}

template<typename T, typename... Arg>
void testConstructNThrowing(const Arg &... arg)
{
	for (auto i : {0, 1, 99})
	{
		g_allocCount.nConstructCalls = 0;
		T::clearCount();
		T::countToThrowOn = i;

		ASSERT_THROW(
			dynarrayTrackingAlloc<T> a(100, arg...),
			TestException );

		ASSERT_EQ(i + 1, g_allocCount.nConstructCalls);
		ASSERT_EQ(i, T::nConstructions);
		ASSERT_EQ(i, T::nDestruct);

		ASSERT_EQ(g_allocCount.nAllocations, g_allocCount.nDeallocations);
	}
}

TEST_F(dynarrayConstructTest, constructForOverwriteThrowing)
{
	testConstructNThrowing<NontrivialConstruct>(for_overwrite);
}

TEST_F(dynarrayConstructTest, constructNThrowing)
{
	testConstructNThrowing<NontrivialConstruct>();
}

TEST_F(dynarrayConstructTest, constructNFillThrowing)
{
	testConstructNThrowing<TrivialRelocat>(TrivialRelocat(-7));
	TrivialRelocat::clearCount(); // testConstructNThrowing messes with counter
}

TEST_F(dynarrayConstructTest, copyConstructThrowing)
{
	for (auto i : {0, 1, 99})
	{
		dynarrayTrackingAlloc<TrivialRelocat> a(100, TrivialRelocat{0.5});

		g_allocCount.nConstructCalls = 0;
		TrivialRelocat::clearCount();
		TrivialRelocat::countToThrowOn = i;

		auto const nExpectAlloc = g_allocCount.nAllocations + 1;

		ASSERT_THROW(
			auto b(a),
			TestException );

		ASSERT_EQ(i + 1, g_allocCount.nConstructCalls);
		ASSERT_EQ(i, TrivialRelocat::nConstructions);
		ASSERT_EQ(i, TrivialRelocat::nDestruct);

		ASSERT_EQ(nExpectAlloc, g_allocCount.nAllocations);
	}
	ASSERT_EQ(g_allocCount.nAllocations, g_allocCount.nDeallocations);

	TrivialRelocat::clearCount();
	g_allocCount.clear();
}
#endif

TEST_F(dynarrayConstructTest, swap)
{
	dynarrayTrackingAlloc<int> a, b{1, 2};
	auto const p = a.data();
	const auto & r = b.back();

	swap(a, b);
	EXPECT_EQ(b.data(), p);
	EXPECT_EQ(2, r);
	EXPECT_EQ(&a.back(), &r);

	b.swap(a);
	EXPECT_EQ(a.data(), p);
	EXPECT_EQ(2, r);
	EXPECT_EQ(&b.back(), &r);
}

#if OEL_MEM_BOUND_DEBUG_LVL

TEST_F(dynarrayConstructTest, swapUnequal)
{
	leakDetector->enabled = false;

	using Al = StatefulAllocator<int>;
	dynarray< int, Al > one(Al(1));
	dynarray< int, Al > two(Al(2));
	ASSERT_DEATH( swap(one, two), "" );
}
#endif

TEST_F(dynarrayConstructTest, rebind)
{
	auto i = -12;
	int * p{};
	dynarray< int *, std::allocator<std::byte> > a, b{p, &i};
	auto const data = b.data();
	auto const back = b.back();

	a = std::move(b);
	EXPECT_EQ(data, a.data());
	EXPECT_EQ(&i, a.back());

	swap(b, a);
	EXPECT_EQ(data, b.data());
	EXPECT_EQ(&i, back);
}