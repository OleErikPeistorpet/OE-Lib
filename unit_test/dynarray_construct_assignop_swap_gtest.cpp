#include "throw_from_assert.h"

#include "dynarray.h"
#include "test_classes.h"

#include <array>
#include <string>


int MyCounter::nConstructions;
int MyCounter::nDestruct;
int MyCounter::countToThrowOn = -1;

int AllocCounter::nAllocations;
int AllocCounter::nDeallocations;
int AllocCounter::nConstructCalls;
std::unordered_map<void *, std::size_t> AllocCounter::sizeFromPtr;

using namespace oel;

template<typename T>
using dynarrayTrackingAlloc = dynarray< T, TrackingAllocator<T> >;

class dynarrayConstructTest : public ::testing::Test
{
protected:
	std::array<unsigned, 3> sizes;

	dynarrayConstructTest()
	{
		AllocCounter::ClearAll();
		MyCounter::ClearCount();

		sizes = {{0, 1, 200}};
	}

	template<typename T>
	void testFillTrivial(T val)
	{
		dynarrayTrackingAlloc<T> a(11, val);

		ASSERT_EQ(a.size(), 11U);
		for (const auto & e : a)
			EXPECT_EQ(val, e);

		ASSERT_EQ(AllocCounter::nDeallocations + 1, AllocCounter::nAllocations);
	}
};

namespace
{
dynarray<int> shouldGetStaticInit;

struct NonConstexprAlloc : oel::allocator<int>
{
	NonConstexprAlloc() {}
	NonConstexprAlloc(const NonConstexprAlloc &) : allocator<int>() {}
};
}

TEST_F(dynarrayConstructTest, nonConstexprCompileTest)
{
	dynarray<int, NonConstexprAlloc> d;
}

TEST_F(dynarrayConstructTest, emptyBracesArg)
{
	dynarray<int> ints({}, 1);
	EXPECT_TRUE(ints.empty());
}

TEST_F(dynarrayConstructTest, constructEmpty)
{
	dynarrayTrackingAlloc<TrivialDefaultConstruct> a;

	ASSERT_TRUE(a.get_allocator() == allocator<TrivialDefaultConstruct>());
	EXPECT_EQ(0U, a.capacity());
	EXPECT_EQ(0, AllocCounter::nAllocations);
	ASSERT_EQ(0, AllocCounter::nDeallocations);
}

#if defined _CPPUNWIND or defined __EXCEPTIONS
TEST_F(dynarrayConstructTest, greaterThanMax)
{
	struct Size2
	{
		char a[2];
	};

	using Test = dynarray<Size2>;
#ifndef _MSC_VER
	const // unreachable code warnings
#endif
		size_t n = std::numeric_limits<size_t>::max() / 2 + 1;

	EXPECT_THROW(Test d(reserve, n), std::length_error);
	EXPECT_THROW(Test d(n, default_init), std::length_error);
	EXPECT_THROW(Test d(n), std::length_error);
	EXPECT_THROW(Test d(n, Size2{{}}), std::length_error);
}
#endif

TEST_F(dynarrayConstructTest, constructReserve)
{
	for (auto const n : sizes)
	{
		auto const nExpectAlloc = AllocCounter::nAllocations + 1;

		dynarrayTrackingAlloc<TrivialDefaultConstruct> a(reserve, n);

		ASSERT_TRUE(a.empty());
		ASSERT_TRUE(a.capacity() >= n);

		if (n > 0)
		{	ASSERT_EQ(nExpectAlloc, AllocCounter::nAllocations); }
	}
	ASSERT_EQ(AllocCounter::nAllocations, AllocCounter::nDeallocations);

	ASSERT_EQ(0, AllocCounter::nConstructCalls);
}

TEST_F(dynarrayConstructTest, constructNDefaultTrivial)
{
	for (auto const n : sizes)
	{
		auto const nExpectAlloc = AllocCounter::nAllocations + 1;

		dynarrayTrackingAlloc<TrivialDefaultConstruct> a(n, default_init);

		ASSERT_EQ(a.size(), n);

		if (n > 0)
		{	ASSERT_EQ(nExpectAlloc, AllocCounter::nAllocations); }
	}
	ASSERT_EQ(AllocCounter::nAllocations, AllocCounter::nDeallocations);

	EXPECT_EQ(0, AllocCounter::nConstructCalls);
}

TEST_F(dynarrayConstructTest, constructNDefault)
{
	for (auto const n : sizes)
	{
		AllocCounter::nConstructCalls = 0;
		NontrivialConstruct::ClearCount();

		auto const nExpectAlloc = AllocCounter::nAllocations + 1;
		{
			dynarrayTrackingAlloc<NontrivialConstruct> a(n, default_init);

			ASSERT_EQ(as_signed(n), AllocCounter::nConstructCalls);
			ASSERT_EQ(as_signed(n), NontrivialConstruct::nConstructions);

			ASSERT_EQ(a.size(), n);

			if (n > 0)
			{	ASSERT_EQ(nExpectAlloc, AllocCounter::nAllocations); }
		}
		ASSERT_EQ(NontrivialConstruct::nConstructions, NontrivialConstruct::nDestruct);
	}
	ASSERT_EQ(AllocCounter::nAllocations, AllocCounter::nDeallocations);
}

TEST_F(dynarrayConstructTest, constructN)
{
	for (auto const n : sizes)
	{
		AllocCounter::nConstructCalls = 0;

		auto const nExpectAlloc = AllocCounter::nAllocations + 1;

		dynarrayTrackingAlloc<TrivialDefaultConstruct> a(n);

		ASSERT_EQ(as_signed(n), AllocCounter::nConstructCalls);

		ASSERT_EQ(a.size(), n);

		if (n > 0)
		{	ASSERT_EQ(nExpectAlloc, AllocCounter::nAllocations); }
	}
	ASSERT_EQ(AllocCounter::nAllocations, AllocCounter::nDeallocations);
}

TEST_F(dynarrayConstructTest, constructNChar)
{
	for (auto const n : sizes)
	{
		auto const nExpectAlloc = AllocCounter::nAllocations + 1;

		dynarrayTrackingAlloc<unsigned char> a(n);

		ASSERT_EQ(a.size(), n);

		for (const auto & c : a)
			ASSERT_EQ(0, c);

		if (n > 0)
		{	ASSERT_EQ(nExpectAlloc, AllocCounter::nAllocations); }
	}
	ASSERT_EQ(AllocCounter::nAllocations, AllocCounter::nDeallocations);
}

TEST_F(dynarrayConstructTest, constructNFill)
{
	testFillTrivial<bool>(true);
	testFillTrivial<char>(97);
	testFillTrivial<int>(97);
	{
		dynarrayTrackingAlloc<NontrivialReloc> a(11, NontrivialReloc(97));

		ASSERT_EQ(a.size(), 11U);
		for (const auto & e : a)
			ASSERT_TRUE(97.0 == *e);

		EXPECT_EQ(1 + 11, NontrivialReloc::nConstructions);

		ASSERT_EQ(AllocCounter::nDeallocations + 1, AllocCounter::nAllocations);
	}
	ASSERT_EQ(NontrivialReloc::nConstructions, NontrivialReloc::nDestruct);

	ASSERT_EQ(AllocCounter::nAllocations, AllocCounter::nDeallocations);
}

TEST_F(dynarrayConstructTest, constructInitList)
{
	{
		auto il = {1.2, 3.4, 5.6, 7.8};
		dynarrayTrackingAlloc<double> a(il);

		ASSERT_TRUE(std::equal( a.begin(), a.end(), begin(il) ));
		ASSERT_EQ(4U, a.size());

		ASSERT_EQ(1, AllocCounter::nAllocations);
	}
	{
		dynarrayTrackingAlloc<NontrivialReloc> a(std::initializer_list<NontrivialReloc>{});

		ASSERT_TRUE(a.empty());
		EXPECT_EQ(0, NontrivialReloc::nConstructions);
	}
	ASSERT_EQ(AllocCounter::nAllocations, AllocCounter::nDeallocations);
}

TEST_F(dynarrayConstructTest, constructContiguousRange)
{
	std::string str = "AbCd";
	dynarray<char> test(str);
	EXPECT_TRUE( 0 == str.compare(0, 4, test.data(), test.size()) );
}


template<typename Alloc>
void testMoveConstruct(Alloc a0, Alloc a1)
{
	for (auto const nl : {0, 1, 80})
	{
		dynarray<MoveOnly, Alloc> right(a0);

		for (int i = 0; i < 9; ++i)
			right.emplace_back(0.5);

		auto const nAllocBefore = AllocCounter::nAllocations;

		auto const ptr = right.data();

		dynarray<MoveOnly, Alloc> left(std::move(right), a1);

		EXPECT_TRUE(left.get_allocator() == a1);

		EXPECT_EQ(nAllocBefore, AllocCounter::nAllocations);
		EXPECT_EQ(9, MoveOnly::nConstructions - MoveOnly::nDestruct);

		if (0 == nl)
		{
			ASSERT_TRUE(right.empty());
			EXPECT_EQ(0U, right.capacity());
		}
		ASSERT_EQ(9U, left.size());
		EXPECT_EQ(ptr, left.data());
	}
	EXPECT_EQ(AllocCounter::nAllocations, AllocCounter::nDeallocations);
}

TEST_F(dynarrayConstructTest, moveConstructWithAlloc)
{
	TrackingAllocator<MoveOnly> a;
	testMoveConstruct(a, a);

	testMoveConstruct< StatefulAllocator<MoveOnly, false> >({0}, {0});
}

template<typename T>
void testConstructMoveElements()
{
	AllocCounter::ClearAll();
	T::ClearCount();
	// not propagating, not equal, cannot steal the memory
	for (auto const na : {0, 1, 101})
	{
		using Alloc = StatefulAllocator<T, false>;
		dynarray<T, Alloc> a(reserve, na, Alloc{1});

		for (int i = 0; i < na; ++i)
			a.emplace_back(i + 0.5);

		auto const capBefore = a.capacity();

		auto const nExpectAlloc = AllocCounter::nAllocations + (a.empty() ? 0 : 1);

		dynarray<T, Alloc> b(std::move(a), Alloc{2});

		ASSERT_FALSE(a.get_allocator() == b.get_allocator());

		EXPECT_EQ(1, a.get_allocator().id);
		EXPECT_EQ(2, b.get_allocator().id);

		EXPECT_EQ(nExpectAlloc, AllocCounter::nAllocations);
		OEL_CONST_COND if (is_trivially_relocatable<T>::value)
		{
			EXPECT_EQ(na, T::nConstructions - T::nDestruct);
			EXPECT_TRUE(a.empty());
		}
		else
		{	EXPECT_EQ(b.size(), a.size());
		}
		EXPECT_EQ(capBefore, a.capacity());
		ASSERT_EQ(na, ssize(b));
		for (int i = 0; i < na; ++i)
			EXPECT_TRUE(b[i].get() and *b[i] == i + 0.5);
	}
	EXPECT_EQ(AllocCounter::nConstructCalls, T::nDestruct);
	EXPECT_EQ(AllocCounter::nAllocations, AllocCounter::nDeallocations);
}

TEST_F(dynarrayConstructTest, moveConstructUnequalAlloc)
{
	testConstructMoveElements<MoveOnly>();
	testConstructMoveElements<NontrivialReloc>();
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

		auto const nAllocBefore = AllocCounter::nAllocations;

		auto const ptr = right.data();

		left = std::move(right);

		OEL_CONST_COND if (std::allocator_traits<Alloc>::propagate_on_container_move_assignment::value)
			EXPECT_TRUE(left.get_allocator() == a0);
		else
			EXPECT_TRUE(left.get_allocator() == a1);

		EXPECT_EQ(nAllocBefore, AllocCounter::nAllocations);
		EXPECT_EQ(9, MoveOnly::nConstructions - MoveOnly::nDestruct);

		if (0 == nl)
		{
			ASSERT_TRUE(right.empty());
			EXPECT_EQ(0U, right.capacity());
		}
		ASSERT_EQ(9U, left.size());
		EXPECT_EQ(ptr, left.data());
	}
	EXPECT_EQ(AllocCounter::nAllocations, AllocCounter::nDeallocations);
}

TEST_F(dynarrayConstructTest, moveAssign)
{
	TrackingAllocator<MoveOnly> a;
	testMoveAssign(a, a);

	testMoveAssign< StatefulAllocator<MoveOnly, true> >({0}, {1});
}

template<typename T>
void testAssignMoveElements()
{
	AllocCounter::ClearAll();
	T::ClearCount();
	// not propagating, not equal, cannot steal the memory
	for (auto const na : {0, 1, 101})
	{
		for (auto const nb : {0, 1, 2})
		{
			using Alloc = StatefulAllocator<T, false>;
			dynarray<T, Alloc> a(reserve, na, Alloc{1});
			dynarray<T, Alloc> b(reserve, nb, Alloc{2});

			ASSERT_FALSE(a.get_allocator() == b.get_allocator());

			for (int i = 0; i < na; ++i)
				a.emplace_back(i + 0.5);

			for (int i = 0; i < nb; ++i)
				b.emplace_back(i + 0.5);

			auto const capBefore = a.capacity();

			auto const nExpectAlloc = AllocCounter::nAllocations + (b.size() < a.size() ? 1 : 0);

			b = std::move(a);

			EXPECT_EQ(1, a.get_allocator().id);
			EXPECT_EQ(2, b.get_allocator().id);

			EXPECT_EQ(nExpectAlloc, AllocCounter::nAllocations);
			OEL_CONST_COND if (is_trivially_relocatable<T>::value)
			{
				EXPECT_EQ(na, T::nConstructions - T::nDestruct);
				EXPECT_TRUE(a.empty());
			}
			else
			{	EXPECT_EQ(b.size(), a.size());
			}
			EXPECT_EQ(capBefore, a.capacity());
			ASSERT_EQ(na, ssize(b));
			for (int i = 0; i < na; ++i)
				EXPECT_TRUE(b[i].get() and *b[i] == i + 0.5);
		}
		EXPECT_EQ(AllocCounter::nConstructCalls, T::nDestruct);
		EXPECT_EQ(AllocCounter::nAllocations, AllocCounter::nDeallocations);
	}
}

TEST_F(dynarrayConstructTest, moveAssignNoPropagateAlloc)
{
	using Alloc = StatefulAllocator<MoveOnly, false>;
	testMoveAssign(Alloc{}, Alloc{});

	testAssignMoveElements<MoveOnly>();
	testAssignMoveElements<NontrivialReloc>();
}


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

		auto il = {1, 2, 3, 4};
		d = il;
		d = d;
		EXPECT_EQ(d.size(), il.size());
		EXPECT_TRUE(std::equal( d.begin(), d.end(), begin(il) ));
	}
	{
		dynarrayTrackingAlloc<NontrivialReloc> nt;
		nt = {NontrivialReloc(5)};
		nt = nt;
		EXPECT_EQ(5, *nt[0]);
	}
	ASSERT_EQ(NontrivialReloc::nConstructions, NontrivialReloc::nDestruct);
	EXPECT_EQ(AllocCounter::nAllocations, AllocCounter::nDeallocations);
}


template<typename T, typename... Arg>
void testConstructNThrowing(const Arg &... arg)
{
OEL_WHEN_EXCEPTIONS_ON(
	for (auto i : {0, 1, 99})
	{
		AllocCounter::nConstructCalls = 0;
		T::ClearCount();
		T::countToThrowOn = i;

		ASSERT_THROW(
			dynarrayTrackingAlloc<T> a(100, arg...),
			TestException );

		ASSERT_EQ(i + 1, AllocCounter::nConstructCalls);
		ASSERT_EQ(i, T::nConstructions);
		ASSERT_EQ(i, T::nDestruct);

		ASSERT_EQ(AllocCounter::nAllocations, AllocCounter::nDeallocations);
	}
)
}

OEL_WHEN_EXCEPTIONS_ON(

TEST_F(dynarrayConstructTest, constructNDefaultThrowing)
{
	testConstructNThrowing<NontrivialConstruct>(default_init);
}

TEST_F(dynarrayConstructTest, constructNThrowing)
{
	testConstructNThrowing<NontrivialConstruct>();
}

TEST_F(dynarrayConstructTest, constructNFillThrowing)
{
	testConstructNThrowing<NontrivialReloc>(NontrivialReloc(-7));
}

TEST_F(dynarrayConstructTest, copyConstructThrowing)
{
	dynarrayTrackingAlloc<NontrivialReloc> a(100, NontrivialReloc{0.5});

	AllocCounter::nAllocations = 0;
	for (auto i : {0, 1, 99})
	{
		AllocCounter::nConstructCalls = 0;
		NontrivialReloc::ClearCount();
		NontrivialReloc::countToThrowOn = i;

		auto const nExpectAlloc = AllocCounter::nAllocations + 1;

		ASSERT_THROW(
			auto b(a),
			TestException );

		ASSERT_EQ(i + 1, AllocCounter::nConstructCalls);
		ASSERT_EQ(i, NontrivialReloc::nConstructions);
		ASSERT_EQ(i, NontrivialReloc::nDestruct);

		ASSERT_EQ(nExpectAlloc, AllocCounter::nAllocations);
		ASSERT_EQ(AllocCounter::nAllocations, AllocCounter::nDeallocations);
	}
}
) // OEL_WHEN_EXCEPTIONS_ON

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
	using Al = StatefulAllocator<int>;
	dynarray< int, Al > one(Al(1));
	dynarray< int, Al > two(Al(2));
#if defined _CPPUNWIND or defined __EXCEPTIONS
	EXPECT_THROW( swap(one, two), std::logic_error );
#else
	EXPECT_DEATH( swap(one, two), "" );
#endif
}
#endif
