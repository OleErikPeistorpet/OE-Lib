// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "test_classes.h"
#include "dynarray.h"
#include "optimize_ext/std_variant.h"
#include "view/move.h"

#include "gtest/gtest.h"
#include <deque>

#if HAS_STD_PMR

#include <memory_resource>

static_assert(oel::is_trivially_relocatable< std::pmr::polymorphic_allocator<int> >::value);

#endif

using oel::dynarray;

namespace
{
	using Iter = dynarray<float>::iterator;
	using ConstIter = dynarray<float>::const_iterator;

#if __cpp_lib_ranges >= 201907
	static_assert(std::ranges::contiguous_range< dynarray<float> >);
#endif
#if __cpp_lib_concepts >= 201907
	static_assert(std::contiguous_iterator<Iter>);
	static_assert(std::contiguous_iterator<ConstIter>);
	static_assert(std::sized_sentinel_for<ConstIter, Iter>);
	static_assert(std::sized_sentinel_for<Iter, ConstIter>);
#endif

	static_assert(std::is_same_v<std::iterator_traits<ConstIter>::value_type, float>);

	static_assert(oel::can_memmove_with<Iter, ConstIter>);
	static_assert(oel::can_memmove_with<Iter, const float *>);
	static_assert(oel::can_memmove_with< float *, std::move_iterator<Iter> >);
	static_assert( !oel::can_memmove_with<int *, Iter> );

	static_assert(std::is_trivially_copyable_v<Iter>);
	static_assert(std::is_convertible_v<Iter, ConstIter>);
	static_assert( !std::is_convertible_v<ConstIter, Iter> );

	static_assert(sizeof(dynarray<float>) == 3 * sizeof(float *),
				  "Not critical, this assert can be removed");

	static_assert(oel::allocator_can_realloc< TrackingAllocator<double> >());
	static_assert(!oel::allocator_can_realloc< oel::allocator<MoveOnly> >());
}

void testCompileSomeDynarrayMembers()
{
	dynarray<int> const d{0};
	static_assert(std::is_same_v< decltype(d.get_allocator()), dynarray<int>::allocator_type >);
	dynarray<int>::allocate_size_overhead();
	d.back();
}

TEST(dynarrayOtherTest, zeroBitRepresentation)
{
	{
		void * ptr = nullptr;
		ASSERT_TRUE(0U == reinterpret_cast<std::uintptr_t>(ptr));
	}
	float f = 0.f;
	for (unsigned i = 0; i < sizeof f; ++i)
		ASSERT_TRUE(0U == reinterpret_cast<const unsigned char *>(&f)[i]);
}

TEST(dynarrayOtherTest, reverseIter)
{
	using oel::iter::as_contiguous_address;
	dynarray<int> const d{0};

	EXPECT_TRUE(d.rbegin().base() != d.cbegin());
	EXPECT_TRUE(d.crbegin().base() == d.cend());
	EXPECT_TRUE(d.rend().base()  != d.end());
	EXPECT_TRUE(d.crend().base() == d.begin());
	EXPECT_EQ(d.data(), &*d.crend().base());
	auto pRbeginBase = as_contiguous_address(d.crbegin().base());
	EXPECT_EQ(d.data() + 1, pRbeginBase);
}

TEST(dynarrayOtherTest, compare)
{
	dynarray<int> arr[3];
	arr[0] = {2, 1};
	arr[1] = {2};
	arr[2] = {1, 3};

	std::sort(std::begin(arr), std::end(arr));

	bool b = arr[0] == dynarray<int>{1, 3};
	EXPECT_TRUE(b);
	b = arr[1] == dynarray<int>{2};
	EXPECT_TRUE(b);
	b = arr[2] != dynarray<int>{2, 1};
	EXPECT_FALSE(b);

	EXPECT_TRUE(arr[2] > arr[1]);
	EXPECT_TRUE(arr[1] > arr[0]);
	EXPECT_FALSE(arr[0] > arr[2]);
}

TEST(dynarrayOtherTest, allocAndIterEquality)
{
	oel::allocator<> a;
	ASSERT_FALSE(oel::allocator<>{} != a);

	EXPECT_TRUE(dynarray<int>::const_iterator() == dynarray<int>::iterator{});
}

using MyAllocStr = oel::allocator<std::string>;
static_assert(std::is_trivially_copyable_v<MyAllocStr>);

TEST(dynarrayOtherTest, stdDequeWithOelAlloc)
{
	std::deque<std::string, MyAllocStr> v{"Test"};
	v.emplace_front();
	EXPECT_EQ("Test", v.at(1));
	EXPECT_TRUE(v.front().empty());
}

TEST(dynarrayOtherTest, oelDynarrWithStdAlloc)
{
	MoveOnly::clearCount();
	{
		auto v = dynarray< MoveOnly, std::allocator<MoveOnly> >(oel::reserve, 2);

		v.emplace_back(-1.0);

	#if OEL_HAS_EXCEPTIONS
		MoveOnly::countToThrowOn = 0;
		EXPECT_THROW( v.emplace_back(), TestException );
	#endif
		EXPECT_EQ(1, MoveOnly::nConstructions);
		EXPECT_EQ(0, MoveOnly::nDestruct);

		MoveOnly arr[2] {MoveOnly{1.0}, MoveOnly{2.0}};
		v.assign(oel::view::move(arr));

	#if OEL_HAS_EXCEPTIONS
		MoveOnly::countToThrowOn = 0;
		EXPECT_THROW( v.emplace_back(), TestException );
	#endif
		EXPECT_EQ(2, oel::ssize(v));
		EXPECT_TRUE(1.0 == *v[0]);
		EXPECT_TRUE(2.0 == *v[1]);
	}
	EXPECT_EQ(MoveOnly::nConstructions, MoveOnly::nDestruct);
}

TEST(dynarrayOtherTest, stdVariant)
{
	using Inner = dynarray<char>;
	using V = std::variant<std::unique_ptr<double>, Inner>;
	static_assert(oel::is_trivially_relocatable<V>::value);

	dynarray<V> a;

	a.emplace_back(Inner(oel::from_range, "abc"));
	a.push_back(std::make_unique<double>(3.3));
	a.reserve(9);

	EXPECT_TRUE(std::strcmp( "abc", std::get<Inner>(a[0]).data() ) == 0);
	EXPECT_EQ( 3.3, *std::get<0>(a[1]) );
}

TEST(dynarrayOtherTest, withReferenceWrapper)
{
	dynarray<int> arr[]{ dynarray<int>(2), {0, 0}, {1, 3} };
	dynarray< std::reference_wrapper<const dynarray<int>> > refs{arr[0], arr[1]};
	refs.push_back(arr[2]);
	EXPECT_EQ(3, refs[2].get()[1]);
	EXPECT_TRUE(refs[0] == refs[1]);
	EXPECT_TRUE(refs[1] != refs[2]);
}
