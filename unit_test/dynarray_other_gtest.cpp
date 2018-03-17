#include "dynarray.h"
#include "compat/std_classes_extra.h"
#include "test_classes.h"

#include "gtest/gtest.h"

/// @cond INTERNAL

using oel::dynarray;

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

	static_assert(alignof(oel::aligned_storage_t<32, 16>) == 16, "?");
	static_assert(alignof(oel::aligned_storage_t<64, 64>) == 64, "?");

	static_assert(oel::is_trivially_copyable< std::reference_wrapper<std::string> >::value,
				  "Not critical, this assert can be removed");

	static_assert(sizeof(dynarray<float>) == 3 * sizeof(float *),
				  "Not critical, this assert can be removed");
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

TEST(dynarrayOtherTest, withReferenceWrapper)
{
	dynarray<int> arr[]{ dynarray<int>(/*size*/ 2, 1), {1, 1}, {1, 3} };
	dynarray< std::reference_wrapper<const dynarray<int>> > refs{arr[0], arr[1]};
	refs.push_back(arr[2]);
	EXPECT_EQ(3, refs.at(2).get().at(1));
	EXPECT_TRUE(refs.at(0) == refs.at(1));
	EXPECT_TRUE(refs.at(1) != refs.at(2));
}

/// @endcond
