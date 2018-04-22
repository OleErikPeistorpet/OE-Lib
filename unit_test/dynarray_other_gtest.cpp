#include "dynarray.h"
#include "compat/std_classes_extra.h"
#include "test_classes.h"
#include "range_view.h"

#include "gtest/gtest.h"
#include <deque>

#if defined __has_include
#if __has_include(<Eigen/Dense>)
	#include "compat/eigen_dense.h"

	static_assert(oel::is_trivially_relocatable<Eigen::Rotation2Df>::value, "?");
#endif
#endif

using oel::dynarray;

namespace
{
	using Iter = dynarray<float>::iterator;
	using ConstIter = dynarray<float>::const_iterator;

	static_assert(std::is_same<std::iterator_traits<ConstIter>::value_type, float>::value, "?");

	static_assert(oel::can_memmove_with<Iter, ConstIter>::value, "?");
	static_assert(oel::can_memmove_with<Iter, const float *>::value, "?");
	static_assert(oel::can_memmove_with<float *, ConstIter>::value, "?");
	static_assert( !oel::can_memmove_with<int *, Iter>::value, "?" );

	static_assert(oel::is_trivially_copyable<Iter>::value, "?");
	static_assert(oel::is_trivially_copyable<ConstIter>::value, "?");
	static_assert(std::is_convertible<Iter, ConstIter>::value, "?");
	static_assert( !std::is_convertible<ConstIter, Iter>::value, "?" );

	static_assert(sizeof(dynarray<float>) == 3 * sizeof(float *),
				  "Not critical, this assert can be removed");
}

TEST(dynarrayOtherTest, zeroBitRepresentation)
{
	{
		void * ptr = nullptr;
		void * ptrToPtr = &ptr;
		ASSERT_TRUE(0U == *static_cast<const std::uintptr_t *>(ptrToPtr));
	}
	float f = 0.f;
	for (unsigned i = 0; i < sizeof f; ++i)
		ASSERT_TRUE(0U == reinterpret_cast<const unsigned char *>(&f)[i]);
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

TEST(dynarrayOtherTest, aggregate)
{
	struct Bar
	{
		int a, b;
	};

	dynarray<Bar> d;
	d.emplace_back(1, 2);
	d.emplace(begin(d), -1, -2);
	EXPECT_EQ(-1, d.front().a);
	EXPECT_EQ(-2, d.front().b);
	EXPECT_EQ(1, d.back().a);
	EXPECT_EQ(2, d.back().b);
}

using MyAllocStr = oel::allocator<std::string>;
static_assert(oel::is_trivially_copyable<MyAllocStr>::value, "?");

#if _MSC_VER or __GNUC__ >= 5

TEST(dynarrayOtherTest, stdDequeWithOelAlloc)
{
	std::deque<std::string, MyAllocStr> v{"Test"};
	v.emplace_front();
	EXPECT_EQ("Test", v.at(1));
	EXPECT_TRUE(v.front().empty());
}

TEST(dynarrayOtherTest, oelDynarrWithStdAlloc)
{
	MoveOnly::ClearCount();
	{
		dynarray< MoveOnly, std::allocator<MoveOnly> > v;

		v.emplace_back(-1.0);

	OEL_WHEN_EXCEPTIONS_ON(
		MoveOnly::countToThrowOn = 0;
		EXPECT_THROW( v.emplace_back(), TestException );
	)
		EXPECT_EQ(1, MoveOnly::nConstructions);
		EXPECT_EQ(0, MoveOnly::nDestruct);

		MoveOnly arr[2] {MoveOnly{1.0}, MoveOnly{2.0}};
		v.assign(oel::view::move(arr));

	OEL_WHEN_EXCEPTIONS_ON(
		MoveOnly::countToThrowOn = 0;
		EXPECT_THROW( v.emplace_back(), TestException );
	)
		EXPECT_EQ(2, ssize(v));
		EXPECT_TRUE(1.0 == *v[0]);
		EXPECT_TRUE(2.0 == *v[1]);
	}
	EXPECT_EQ(MoveOnly::nConstructions, MoveOnly::nDestruct);
}
#endif

TEST(dynarrayOtherTest, withReferenceWrapper)
{
	dynarray<int> arr[]{ dynarray<int>(/*size*/ 2, 1), {1, 1}, {1, 3} };
	dynarray< std::reference_wrapper<const dynarray<int>> > refs{arr[0], arr[1]};
	refs.push_back(arr[2]);
	EXPECT_EQ(3, refs.at(2).get().at(1));
	EXPECT_TRUE(refs.at(0) == refs.at(1));
	EXPECT_TRUE(refs.at(1) != refs.at(2));
}
