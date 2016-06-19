#include "forward_decl_test.h"
#include "generic_array_test.h"

/// @cond INTERNAL

class ForwDeclared { char c; };

int MyCounter::nConstruct;
int MyCounter::nDestruct;


using oel::dynarray;
using oel::make_view;
using oel::make_view_n;

namespace statictest
{
	using Iter = dynarray<float>::iterator;
	using ConstIter = dynarray<float>::const_iterator;

	static_assert(std::is_same<std::iterator_traits<ConstIter>::value_type, float>::value, "?");

	static_assert(oel::can_memmove_with<Iter, ConstIter>::value, "?");
	static_assert(oel::can_memmove_with<Iter, const float *>::value, "?");
	static_assert(oel::can_memmove_with<float *, ConstIter>::value, "?");
	static_assert(!oel::can_memmove_with<int *, float *>::value, "?");

	static_assert(oel::is_trivially_copyable<Iter>::value, "?");
	static_assert(oel::is_trivially_copyable<ConstIter>::value, "?");
	static_assert(std::is_convertible<Iter, ConstIter>::value, "?");
	static_assert(! std::is_convertible<ConstIter, Iter>::value, "?");

	static_assert(oel::is_trivially_relocatable< std::array<std::unique_ptr<double>, 4> >::value, "?");

	static_assert(oel::is_trivially_copyable< std::pair<long *, std::array<int, 6>> >::value, "?");
	static_assert(oel::is_trivially_copyable< std::tuple<> >::value, "?");
	static_assert(! oel::is_trivially_copyable< std::tuple<int, dynarray<bool>, int> >::value, "?");

	static_assert(oel::is_trivially_copyable< std::reference_wrapper<std::deque<double>> >::value,
				  "Not critical, this assert can be removed");

	static_assert(sizeof(dynarray<float>) == 3 * sizeof(float *),
				  "Not critical, this assert can be removed");
}

template<typename T>
struct throwingAlloc : public oel::allocator<T>
{
	unsigned int throwIfGreater = 999;

	T * allocate(size_t nObjs)
	{
		if (nObjs > throwIfGreater)
			throw std::bad_alloc{};

		return oel::allocator<T>::allocate(nObjs);
	}
};
static_assert(! oel::is_always_equal_allocator<throwingAlloc<int>>::value, "?");

// The fixture for testing dynarray.
class dynarrayTest : public ::testing::Test
{
protected:
	dynarrayTest()
	{
		// You can do set-up work for each test here.
	}

	// Objects declared here can be used by all tests.
};

using MyAllocStr = oel::allocator<std::string>;
static_assert(oel::is_trivially_copyable<MyAllocStr>::value, "?");

#if _MSC_VER || __GNUC__ >= 5

TEST_F(dynarrayTest, stdDequeWithOelAlloc)
{
	std::deque<std::string, MyAllocStr> v{"Test"};
	v.emplace_front();
	EXPECT_EQ("Test", v.at(1));
	EXPECT_TRUE(v.front().empty());
}

TEST_F(dynarrayTest, oelDynarrWithStdAlloc)
{
	MoveOnly::ClearCount();
	{
		dynarray< MoveOnly, std::allocator<MoveOnly> > v;

		v.emplace_back(-1.0);
		EXPECT_THROW( v.emplace_back(ThrowOnConstruct), TestException );
		EXPECT_EQ(1, MoveOnly::nConstruct);
		EXPECT_EQ(0, MoveOnly::nDestruct);

		MoveOnly arr[2] {MoveOnly{1.0}, MoveOnly{2.0}};
		v.assign(oel::view::move(arr));
		EXPECT_THROW( v.emplace_back(ThrowOnConstruct), TestException );
		EXPECT_EQ(2, ssize(v));
		EXPECT_TRUE(1.0 == *v[0]);
		EXPECT_TRUE(2.0 == *v[1]);
	}
	EXPECT_EQ(MoveOnly::nConstruct, MoveOnly::nDestruct);
}
#endif

TEST_F(dynarrayTest, construct)
{
	{
		oel::allocator<int> a;
		ASSERT_TRUE(oel::allocator<std::string>{} == a);
	}

	{
		Outer o;
	}

	using Internal = std::deque<double *>;
	dynarray<Internal> test{Internal(5), Internal()};
	EXPECT_EQ(2U, test.size());

	testConstruct< dynarray<std::string>, dynarray<char>, dynarray<bool> >();
}

TEST_F(dynarrayTest, pushBack)
{
	testPushBack< dynarray<MoveOnly>, dynarray<dynarray<int>> >();
}

TEST_F(dynarrayTest, pushBackNonTrivialReloc)
{
	testPushBackNonTrivialReloc< dynarray<NontrivialReloc> >();
}

TEST_F(dynarrayTest, assign)
{
	testAssign< dynarray<MoveOnly>, dynarray<NontrivialReloc> >();
}

TEST_F(dynarrayTest, assignStringStream)
{
	testAssignStringStream< dynarray<std::string> >();
}

TEST_F(dynarrayTest, append)
{
	testAppend< oel::dynarray<double>, dynarray<int> >();
}

TEST_F(dynarrayTest, appendFromStringStream)
{
	testAppendFromStringStream< dynarray<int> >();
}

TEST_F(dynarrayTest, insertR)
{
	testInsertR< dynarray<double>, dynarray<int> >();
}

TEST_F(dynarrayTest, insert)
{
	testInsert< dynarray<MoveOnly> >();
}

TEST_F(dynarrayTest, resize)
{
	testResize< dynarray<int, throwingAlloc<int>>, std::bad_alloc >();

	dynarray< dynarray<int> > nested;
	nested.resize(3);
	EXPECT_EQ(3U, nested.size());
	EXPECT_TRUE(nested.back().empty());

	nested.front().resize(4);

	nested.resize(1);
	auto cap = nested.capacity();
	EXPECT_EQ(1U, nested.size());
	for (auto i : nested.front())
		EXPECT_EQ(0, i);

	auto it = nested.begin();

	nested.resize(cap);
	// Check that no reallocation happened
	EXPECT_EQ(cap, nested.capacity());
	EXPECT_TRUE(nested.begin() == it);

	EXPECT_EQ(4, nested.front().size());
	EXPECT_TRUE(nested.back().empty());
}

TEST_F(dynarrayTest, eraseSingle)
{
	testEraseSingle< dynarray<int>, dynarray<NontrivialReloc> >();
}

TEST_F(dynarrayTest, eraseRange)
{
	testEraseRange< dynarray<unsigned int> >();
}

TEST_F(dynarrayTest, eraseToEnd)
{
	testEraseToEnd< dynarray<int> >();
}

#ifndef OEL_NO_BOOST
TEST_F(dynarrayTest, overAligned)
{
	unsigned int const testAlignment = 32;

	dynarray< oel::aligned_storage_t<testAlignment, testAlignment> > special(0);
	EXPECT_TRUE(special.cbegin() == special.cend());

	special.append(5, {});
	EXPECT_EQ(5U, special.size());
	for (const auto & v : special)
		EXPECT_EQ(0U, reinterpret_cast<std::uintptr_t>(&v) % testAlignment);

	special.resize(1, oel::default_init);
	special.shrink_to_fit();
	EXPECT_GE(5U, special.capacity());
	EXPECT_EQ(0U, reinterpret_cast<std::uintptr_t>(&special.front()) % testAlignment);
}
#endif

TEST_F(dynarrayTest, withRefWrapper)
{
	testWithRefWrapper<	dynarray<int>, dynarray<std::reference_wrapper<dynarray<int> const>> >();
}

TEST_F(dynarrayTest, misc)
{
	size_t fASrc[] = { 2, 3 };

	dynarray<size_t> daSrc(oel::reserve, 2);
	daSrc.push_back(0);
	daSrc.push_back(2);
	daSrc.insert(begin(daSrc) + 1, 1);
	ASSERT_EQ(3U, daSrc.size());

	ASSERT_NO_THROW(daSrc.at(2));
	ASSERT_THROW(daSrc.at(3), std::out_of_range);

	std::deque<size_t> dequeSrc;
	dequeSrc.push_back(4);
	dequeSrc.push_back(5);

	dynarray<size_t> dest0;
	dest0.reserve(1);
	dest0 = daSrc;

	dest0.append_ret_src( make_view_n(cbegin(daSrc), daSrc.size()) );
	dest0.append(make_view_n(fASrc, 2));
	auto srcEnd = dest0.append_ret_src( make_view_n(dequeSrc.begin(), dequeSrc.size()) );
	EXPECT_TRUE(end(dequeSrc) == srcEnd);

	dynarray<size_t> dest1;
	dynarray<size_t>::const_iterator( dest1.append(daSrc) );
	dest1.append(fASrc);
	dest1.append(dequeSrc);

	auto cap = dest1.capacity();
	dest1.pop_back();
	dest1.pop_back();
	dest1.shrink_to_fit();
	EXPECT_GT(cap, dest1.capacity());

	{
		dynarray<int> di{1, -2};
		auto it = begin(di);
		it = di.erase_unordered(it);
		EXPECT_EQ(-2, *it);
		it = di.erase_unordered(it);
		EXPECT_EQ(end(di), it);

		di = {1, -2};
		erase_unordered(di, 1);
		erase_unordered(di, 0);
		EXPECT_TRUE(di.empty());
	}
}

/// @endcond
