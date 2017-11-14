#include "dynarray.h"
#include "test_classes.h"
#include "compat/std_classes_extra.h"

#include <string>

/// @cond INTERNAL

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
	static_assert(_detail::is_trivially_default_constructible<TrivialDefaultConstruct>::value, "?");
	static_assert( !_detail::is_trivially_default_constructible<NontrivialConstruct>::value, "?" );

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
	using Iter = dynarray<float>::iterator;
	using ConstIter = dynarray<float>::const_iterator;

	static_assert(std::is_same<std::iterator_traits<ConstIter>::value_type, float>::value, "?");

	static_assert(oel::can_memmove_with<Iter, ConstIter>::value, "?");
	static_assert(oel::can_memmove_with<Iter, const float *>::value, "?");
	static_assert(oel::can_memmove_with<float *, ConstIter>::value, "?");
	static_assert( !oel::can_memmove_with<int *, float *>::value, "?" );

	static_assert(oel::is_trivially_copyable<Iter>::value, "?");
	static_assert(oel::is_trivially_copyable<ConstIter>::value, "?");
	static_assert(std::is_convertible<Iter, ConstIter>::value, "?");
	static_assert( !std::is_convertible<ConstIter, Iter>::value, "?" );

	static_assert(oel::is_trivially_relocatable< std::array<std::unique_ptr<double>, 4> >::value, "?");

	static_assert(oel::is_trivially_copyable< std::pair<long *, std::array<int, 6>> >::value, "?");
	static_assert(oel::is_trivially_copyable< std::tuple<> >::value, "?");
	static_assert( !oel::is_trivially_copyable< std::tuple<int, NontrivialReloc, int> >::value, "?" );

	static_assert(OEL_ALIGNOF(oel::aligned_storage_t<32, 16>) == 16, "?");
	static_assert(OEL_ALIGNOF(oel::aligned_storage_t<64, 64>) == 64, "?");

	static_assert(oel::is_trivially_copyable< std::reference_wrapper<std::string> >::value,
				  "Not critical, this assert can be removed");

	static_assert(sizeof(dynarray<float>) == 3 * sizeof(float *),
				  "Not critical, this assert can be removed");
}

TEST_F(dynarrayConstructTest, misc)
{
	{
		oel::allocator<int> a;
		ASSERT_TRUE(oel::allocator<std::string>{} == a);
	}

	EXPECT_TRUE(dynarray<int>::const_iterator{} == dynarray<int>::iterator{});

	dynarray<int> ints(0, {});
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

TEST_F(dynarrayConstructTest, constructReserve)
{
	for (auto const n : sizes)
	{
		auto const nExpectAlloc = AllocCounter::nAllocations + 1;

		dynarrayTrackingAlloc<TrivialDefaultConstruct> a(reserve, n);

		ASSERT_TRUE(a.empty());
		ASSERT_TRUE(a.capacity() >= n);

		if (n > 0)
			ASSERT_EQ(nExpectAlloc, AllocCounter::nAllocations);
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
			ASSERT_EQ(nExpectAlloc, AllocCounter::nAllocations);
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
				ASSERT_EQ(nExpectAlloc, AllocCounter::nAllocations);
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
			ASSERT_EQ(nExpectAlloc, AllocCounter::nAllocations);
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
			ASSERT_EQ(nExpectAlloc, AllocCounter::nAllocations);
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
			ASSERT_TRUE(97.0 == e);

		EXPECT_EQ(1 + 11, NontrivialReloc::nConstructions);

		ASSERT_EQ(AllocCounter::nDeallocations + 1, AllocCounter::nAllocations);
	}
	ASSERT_EQ(NontrivialReloc::nConstructions, NontrivialReloc::nDestruct);

	ASSERT_EQ(AllocCounter::nAllocations, AllocCounter::nDeallocations);
}

TEST_F(dynarrayConstructTest, constructInitList)
{
	{
		dynarrayTrackingAlloc<double> a{1.2, 3.4, 5.6, 7.8};

		ASSERT_TRUE(std::equal( a.begin(), a.end(), begin({1.2, 3.4, 5.6, 7.8}) ));
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

TEST_F(dynarrayConstructTest, moveAssignNoPropagateAlloc)
{
	using Alloc = StatefulAllocator<MoveOnly, false>;

	testMoveAssign(Alloc{}, Alloc{});

	MoveOnly::ClearCount();
	// not propagating, not equal, cannot steal the memory
	for (auto const na : {0, 1, 101})
		for (auto const nb : {0, 1, 2})
		{
			dynarray<MoveOnly, Alloc> a(reserve, na, Alloc{1});
			dynarray<MoveOnly, Alloc> b(reserve, nb, Alloc{2});

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
			EXPECT_EQ(na, MoveOnly::nConstructions - MoveOnly::nDestruct);

			EXPECT_TRUE(a.empty());
			EXPECT_EQ(capBefore, a.capacity());
			ASSERT_EQ(na, ssize(b));
			for (int i = 0; i < na; ++i)
				EXPECT_TRUE(b[i].get() && *b[i] == i + 0.5);
		}

	EXPECT_EQ(AllocCounter::nAllocations, AllocCounter::nDeallocations);
}

TEST_F(dynarrayConstructTest, selfMoveAssign)
{
	dynarray<int> d(4, -3);
	{
		auto tmp = std::move(d);
		d = std::move(d);
		d = std::move(tmp);
	}
	EXPECT_EQ(4U, d.size());
	EXPECT_EQ(-3, d.back());
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

/// @endcond
