#include "dynarray.h"
#include "test_classes.h"
#include "util.h"
#include "compat/std_classes_extra.h"

#include <string>

/// @cond INTERNAL

int MyCounter::nConstructions;
int MyCounter::nDestruct;

int AllocCounter::nAllocations;
int AllocCounter::nDeallocations;
int AllocCounter::nConstructCalls;

using namespace oel;

template<typename T>
using dynarrayTrackingAlloc = dynarray< T, TrackingAllocator<T> >;

class dynarrayConstructTest : public ::testing::Test
{
protected:
	dynarrayConstructTest()
	{
		AllocCounter::nAllocations = 0;
		AllocCounter::nDeallocations = 0;
		AllocCounter::nConstructCalls = 0;

		MyCounter::ClearCount();
	}

	template<typename T>
	void testFillTrivial(T val)
	{
		dynarrayTrackingAlloc<T> a(11, val);

		ASSERT_EQ(a.size(), 11U);
		for (const auto & e : a)
			EXPECT_EQ(val, e);
	}

	std::array<unsigned, 3> sizes = {{0, 1, 200}};
};

struct TrivialDefaultConstruct
{
	TrivialDefaultConstruct() = default;
	TrivialDefaultConstruct(TrivialDefaultConstruct &&) = delete;
	TrivialDefaultConstruct(const TrivialDefaultConstruct &) = delete;
};
static_assert(_detail::is_trivially_default_constructible<TrivialDefaultConstruct>::value, "?");

struct NontrivialConstruct : MyCounter
{
	NontrivialConstruct(const NontrivialConstruct &) = delete;
	NontrivialConstruct() { ++nConstructions; }
	~NontrivialConstruct() { ++nDestruct; }
};
static_assert( !_detail::is_trivially_default_constructible<NontrivialConstruct>::value, "?" );

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
	static_assert( !oel::is_trivially_copyable< std::tuple<int, dynarray<bool>, int> >::value, "?" );

	static_assert(OEL_ALIGNOF(oel::aligned_storage_t<32, 16>) == 16, "?");
	static_assert(OEL_ALIGNOF(oel::aligned_storage_t<64, 64>) == 64, "?");

	static_assert(oel::is_trivially_copyable< std::reference_wrapper<std::string> >::value,
				  "Not critical, this assert can be removed");

	static_assert(sizeof(dynarray<float>) == 3 * sizeof(float *),
				  "Not critical, this assert can be removed");
}

TEST_F(dynarrayConstructTest, misc)
{
	// TODO: Test exception safety of constructors

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
	for (int i = 0; i < ssize(sizes); ++i)
	{
		auto const n = sizes[i];

		dynarrayTrackingAlloc<TrivialDefaultConstruct> a(reserve, n);

		ASSERT_TRUE(a.empty());
		ASSERT_TRUE(a.capacity() >= n);

		ASSERT_EQ(i + 1, AllocCounter::nAllocations);
		ASSERT_EQ(i, AllocCounter::nDeallocations);
	}
	ASSERT_EQ(AllocCounter::nAllocations, AllocCounter::nDeallocations);

	ASSERT_EQ(0, AllocCounter::nConstructCalls);
}

TEST_F(dynarrayConstructTest, constructNDefaultTrivial)
{
	for (int i = 0; i < ssize(sizes); ++i)
	{
		auto const n = sizes[i];

		dynarrayTrackingAlloc<TrivialDefaultConstruct> a(n, default_init);

		ASSERT_EQ(a.size(), n);

		ASSERT_EQ(i + 1, AllocCounter::nAllocations);
		ASSERT_EQ(i, AllocCounter::nDeallocations);
	}
	ASSERT_EQ(AllocCounter::nAllocations, AllocCounter::nDeallocations);

	EXPECT_EQ(0, AllocCounter::nConstructCalls);
}

TEST_F(dynarrayConstructTest, constructNDefault)
{
	for (int i = 0; i < ssize(sizes); ++i)
	{
		auto const n = sizes[i];

		AllocCounter::nConstructCalls = 0;
		NontrivialConstruct::ClearCount();
		{
			dynarrayTrackingAlloc<NontrivialConstruct> a(n, default_init);

			ASSERT_EQ(as_signed(n), AllocCounter::nConstructCalls);
			ASSERT_EQ(as_signed(n), NontrivialConstruct::nConstructions);

			ASSERT_EQ(a.size(), n);

			ASSERT_EQ(i + 1, AllocCounter::nAllocations);
			ASSERT_EQ(i, AllocCounter::nDeallocations);
		}
		ASSERT_EQ(NontrivialConstruct::nConstructions, NontrivialConstruct::nDestruct);
	}
	ASSERT_EQ(AllocCounter::nAllocations, AllocCounter::nDeallocations);
}

TEST_F(dynarrayConstructTest, constructN)
{
	for (int i = 0; i < ssize(sizes); ++i)
	{
		auto const n = sizes[i];

		AllocCounter::nConstructCalls = 0;

		dynarrayTrackingAlloc<TrivialDefaultConstruct> a(n);

		ASSERT_EQ(as_signed(n), AllocCounter::nConstructCalls);

		ASSERT_EQ(a.size(), n);

		ASSERT_EQ(i + 1, AllocCounter::nAllocations);
		ASSERT_EQ(i, AllocCounter::nDeallocations);
	}
	ASSERT_EQ(AllocCounter::nAllocations, AllocCounter::nDeallocations);
}

TEST_F(dynarrayConstructTest, constructNChar)
{
	for (int i = 0; i < ssize(sizes); ++i)
	{
		auto const n = sizes[i];

		dynarrayTrackingAlloc<unsigned char> a(n);

		ASSERT_EQ(a.size(), n);

		for (const auto & c : a)
			ASSERT_EQ(0, c);

		ASSERT_EQ(i + 1, AllocCounter::nAllocations);
		ASSERT_EQ(i, AllocCounter::nDeallocations);
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
			ASSERT_EQ(97, e);

		EXPECT_EQ(1 + 11, NontrivialReloc::nConstructions);

		ASSERT_EQ(4, AllocCounter::nAllocations);
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
	}
	{
		dynarrayTrackingAlloc<NontrivialReloc> a(std::initializer_list<NontrivialReloc>{});
		ASSERT_TRUE(a.empty());

		EXPECT_EQ(0, NontrivialReloc::nConstructions);

		ASSERT_EQ(2, AllocCounter::nAllocations);
	}
	ASSERT_EQ(AllocCounter::nAllocations, AllocCounter::nDeallocations);
}

TEST_F(dynarrayConstructTest, constructContiguousRange)
{
	std::string str = "AbCd";
	dynarray<char> test(str);
	EXPECT_TRUE( 0 == str.compare(0, 4, test.data(), test.size()) );
}

/// @endcond
