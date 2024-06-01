// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "test_classes.h"
#include "mem_leak_detector.h"
#include "view/move.h"
#include "dynarray.h"

#include <deque>
#include <array>


using oel::dynarray;
namespace view = oel::view;

template< typename T = int >
struct noDefaultConstructAlloc : public oel::allocator<T>
{
	noDefaultConstructAlloc(int) {}
};

template<typename T>
struct throwingAlloc : public oel::allocator<T>
{
	unsigned int throwIfGreater = 999;

	T * allocate(size_t nObjs)
	{
		if (nObjs > throwIfGreater)
			OEL_THROW(std::bad_alloc{}, "");

		return oel::allocator<T>::allocate(nObjs);
	}
};

// The fixture for testing dynarray.
class dynarrayTest : public ::testing::Test
{
protected:
	~dynarrayTest()
	{
		EXPECT_EQ(g_allocCount.nAllocations, g_allocCount.nDeallocations);
		EXPECT_EQ(MyCounter::nConstructions, MyCounter::nDestruct);

		g_allocCount.clear();
		MyCounter::clearCount();
	}
};

template< typename T >
void testPushBack1()
{
	T::clearCount();
	{
		dynarray<T> da;

		double const VALUES[] = {-1.1, 2.0};

		da.push_back(T{VALUES[0]});
		ASSERT_EQ(1U, da.size());

	#if OEL_HAS_EXCEPTIONS
		T::countToThrowOn = 0;
		EXPECT_THROW( da.emplace_back(), TestException );
		ASSERT_EQ(1U, da.size());
	#endif
		da.push_back(T{VALUES[1]});
		ASSERT_EQ(2U, da.size());

	#if OEL_HAS_EXCEPTIONS
		T::countToThrowOn = 0;
		EXPECT_THROW( da.emplace_back(), TestException );
		ASSERT_EQ(2U, da.size());
	#endif
		da.reserve(3);
		da.push_back( std::move(da.back()) );
		ASSERT_EQ(3U, da.size());

		EXPECT_EQ(VALUES[0], *da[0]);
		if (std::is_same<T, MoveOnly>::value)
			EXPECT_FALSE(da[1].hasValue());
		else
			EXPECT_EQ(VALUES[1], *da[1]);

		EXPECT_EQ(VALUES[1], *da[2]);
	}
	EXPECT_EQ(T::nConstructions, T::nDestruct);
}

TEST_F(dynarrayTest, pushBackCase1)
{
	testPushBack1<MoveOnly>();
	testPushBack1<TrivialRelocat>();
}

TEST_F(dynarrayTest, emplaceBackNested)
{
	dynarray< dynarray<int> > nested;
	nested.emplace_back(3, oel::for_overwrite);
	EXPECT_EQ(3U, nested.back().size());
	auto & ret = nested.emplace_back(std::initializer_list<int>{1, 2});
	EXPECT_EQ(2U, nested.back().size());
	EXPECT_TRUE(ret == nested.back());
}

template< typename T >
void testPushBack2()
{
	T::clearCount();
	{
		dynarray<T> da;

		double const VALUES[] = {-1.1, 2.0, -0.7, 9.6};
		std::deque<double> expected;

		da.push_back(T{VALUES[0]});
		expected.push_back(VALUES[0]);
		ASSERT_EQ(1U, da.size());
		EXPECT_EQ(T::nConstructions - ssize(da), T::nDestruct);

		auto & ret = da.emplace_back(VALUES[1]);
		expected.emplace_back(VALUES[1]);
		ASSERT_EQ(&da.back(), &ret);
		ASSERT_EQ(2U, da.size());
		EXPECT_EQ(T::nConstructions - ssize(da), T::nDestruct);

	#if OEL_HAS_EXCEPTIONS
		T::countToThrowOn = 1;
		try
		{
			for(;;)
			{
				da.push_back(T{VALUES[2]});
				expected.push_back(VALUES[2]);
			}
		}
		catch (TestException &) {
		}
		ASSERT_EQ(expected.size(), da.size());
		EXPECT_EQ(T::nConstructions - ssize(da), T::nDestruct);
	#endif
		da.emplace_back(VALUES[3]);
		expected.emplace_back(VALUES[3]);
		ASSERT_EQ(expected.size(), da.size());

	#if OEL_HAS_EXCEPTIONS
		T::countToThrowOn = 0;
		EXPECT_THROW( da.emplace_back(0), TestException );
		ASSERT_EQ(expected.size(), da.size());
	#endif
		EXPECT_EQ(T::nConstructions - ssize(da), T::nDestruct);

	#if OEL_HAS_EXCEPTIONS
		da.reserve(da.size() + 2);
		T::countToThrowOn = 1;
		try
		{
			for(;;)
			{
				da.push_back(T{*da.front()});
				expected.push_back(expected.front());
			}
		}
		catch (TestException &) {
		}
		ASSERT_EQ(expected.size(), da.size());
	#endif
		EXPECT_TRUE(
			std::equal(
				begin(da), end(da), begin(expected),
				[](const T & a, double b) { return *a == b; } ) );
	}
	EXPECT_EQ(T::nConstructions, T::nDestruct);
}

TEST_F(dynarrayTest, pushBackCase2)
{
	testPushBack2<MoveOnly>();
	testPushBack2<TrivialRelocat>();
}

static int funcToRef() { return 0; }

struct ConstructFromRef
{
	using UPtr = std::unique_ptr<double>;

	template< typename T >
	ConstructFromRef(UPtr &, T &&, int(&)())
	{
		static_assert(std::is_const<T>());
	}
};

TEST_F(dynarrayTest, emplaceRefTypePreserve)
{
	dynarray<ConstructFromRef> d;
	ConstructFromRef::UPtr p0{};
	ConstructFromRef::UPtr const p1{};
	d.emplace_back(p0, std::move(p1), funcToRef);
	d.emplace(d.begin(), p0, std::move(p1), funcToRef);
}

TEST_F(dynarrayTest, assign)
{
	double const VALUES[] = {-1.1, 0.4};
	MoveOnly src[] { MoveOnly{VALUES[0]},
	                 MoveOnly{VALUES[1]} };
	dynarray<MoveOnly> test;

	test.assign(view::move(src));

	EXPECT_EQ(2U, test.size());
	EXPECT_EQ(VALUES[0], *test[0]);
	EXPECT_EQ(VALUES[1], *test[1]);

	test.assign(view::subrange(src, src) | view::move);
	EXPECT_EQ(0U, test.size());
}

TEST_F(dynarrayTest, assignTrivialReloc)
{
	dynarray<TrivialRelocat> dest;
	#if OEL_HAS_EXCEPTIONS
	{
		TrivialRelocat obj{0};
		TrivialRelocat::countToThrowOn = 0;
		EXPECT_THROW(
			dest.assign(view::counted(&obj, 1)),
			TestException );
		EXPECT_TRUE(dest.begin() == dest.end());
	}
	#endif
	EXPECT_EQ(TrivialRelocat::nConstructions, TrivialRelocat::nDestruct);

	dest = {TrivialRelocat{-1.0}};
	EXPECT_EQ(1U, dest.size());
	dest = {TrivialRelocat{1.0}, TrivialRelocat{2.0}};
	EXPECT_EQ(1.0, *dest.at(0));
	EXPECT_EQ(2.0, *dest.at(1));
	EXPECT_EQ(TrivialRelocat::nConstructions - ssize(dest), TrivialRelocat::nDestruct);
	#if OEL_HAS_EXCEPTIONS
	{
		TrivialRelocat obj{0};
		TrivialRelocat::countToThrowOn = 0;
		EXPECT_THROW(
			dest.assign(view::subrange(&obj, &obj + 1)),
			TestException );
		EXPECT_TRUE(dest.empty() or *dest.at(1) == 2.0);
	}
	#endif
	{
		dest.clear();
		EXPECT_LE(2U, dest.capacity());
		EXPECT_TRUE(dest.empty());

	#if OEL_HAS_EXCEPTIONS
		TrivialRelocat obj{0};
		TrivialRelocat::countToThrowOn = 0;
		EXPECT_THROW(
			dest.assign(view::counted(&obj, 1)),
			TestException );
		EXPECT_TRUE(dest.empty());
	#endif
	}
}

// std::stringstream doesn't seem to work using libstdc++ with -fno-exceptions
// Probably needs a -fno-exceptions build of libstdc++
#if !defined __GLIBCXX__ or OEL_HAS_EXCEPTIONS
TEST_F(dynarrayTest, assignNonForwardRange)
{
	dynarrayTrackingAlloc<std::string> das;

	std::string * p = nullptr;
	das.assign(view::subrange(p, p));

	EXPECT_EQ(0U, das.size());

	std::stringstream ss{"My computer emits Hawking radiation"};
	std::istream_iterator<std::string> b{ss}, e;
	das.assign(view::subrange(b, e));

	EXPECT_EQ(5U, das.size());

	EXPECT_EQ("My", das.at(0));
	EXPECT_EQ("computer", das.at(1));
	EXPECT_EQ("emits", das.at(2));
	EXPECT_EQ("Hawking", das.at(3));
	EXPECT_EQ("radiation", das.at(4));

	decltype(das) copyDest;

	copyDest.assign(view::counted(das.cbegin(), 2));
	copyDest.assign( view::counted(begin(das), das.size()) );

	EXPECT_TRUE(das == copyDest);

	copyDest.assign(view::subrange(das.cbegin(), das.cbegin() + 1));

	EXPECT_EQ(1U, copyDest.size());
	EXPECT_EQ(das[0], copyDest[0]);

	copyDest.assign(view::counted(das.cbegin() + 2, 3));

	EXPECT_EQ(3U, copyDest.size());
	EXPECT_EQ(das[2], copyDest[0]);
	EXPECT_EQ(das[3], copyDest[1]);
	EXPECT_EQ(das[4], copyDest[2]);

	copyDest = {std::string()};
	EXPECT_EQ("", copyDest.at(0));
	copyDest = {das[0], das[4]};
	EXPECT_EQ(2U, copyDest.size());
	EXPECT_EQ(das[4], copyDest.at(1));

	copyDest = std::initializer_list<std::string>{};
	EXPECT_TRUE(copyDest.empty());
}
#endif

TEST_F(dynarrayTest, appendCase1)
{
	oel::dynarray<double> dest;
	// Test append empty std iterator range to empty dynarray
	std::deque<double> src;
	dest.append(src);

	dest.append({});
	EXPECT_EQ(0U, dest.size());

	double const TEST_VAL = 6.6;
	dest.append(2, TEST_VAL);
	dest.reserve(2 * dest.size());
	dest.append( view::subrange(dest.begin(), dest.end()) );
	EXPECT_EQ(4U, dest.size());
	for (const auto & d : dest)
		EXPECT_EQ(TEST_VAL, d);
}

TEST_F(dynarrayTest, appendCase2)
{
	const double arrayA[] = {-1.6, -2.6, -3.6, -4.6};

	dynarray<double> double_dynarr, double_dynarr2;
	double_dynarr.append( view::counted(oel::begin(arrayA), oel::ssize(arrayA)) );
	double_dynarr.append(double_dynarr2);

	{
		dynarray<int> int_dynarr;
		int_dynarr.append({1, 2, 3, 4});

		double_dynarr.append(int_dynarr);
	}

	ASSERT_EQ(8U, double_dynarr.size());

	EXPECT_EQ(arrayA[0], double_dynarr[0]);
	EXPECT_EQ(arrayA[1], double_dynarr[1]);
	EXPECT_EQ(arrayA[2], double_dynarr[2]);
	EXPECT_EQ(arrayA[3], double_dynarr[3]);

	EXPECT_DOUBLE_EQ(1, double_dynarr[4]);
	EXPECT_DOUBLE_EQ(2, double_dynarr[5]);
	EXPECT_DOUBLE_EQ(3, double_dynarr[6]);
	EXPECT_DOUBLE_EQ(4, double_dynarr[7]);
}

#if OEL_HAS_EXCEPTIONS
TEST_F(dynarrayTest, appendSizeOverflow)
{
	dynarray<char> c(1);
	EXPECT_THROW(
		c.append(SIZE_MAX, '\0'),
		std::length_error );
}
#endif

#if !defined __GLIBCXX__ or OEL_HAS_EXCEPTIONS
TEST_F(dynarrayTest, appendNonForwardRange)
{
	std::stringstream ss("1 2 3");

	dynarrayTrackingAlloc<int> dest;

	std::istream_iterator<int> it(ss), end{};

	it = dest.append(view::counted(it, 0));
	it = dest.append(view::counted(it, 2));
	it = dest.append(view::counted(it, 0));

	it = dest.append(view::subrange(it, end));

	EXPECT_EQ(it, end);
	EXPECT_EQ(3u, dest.size());
	for (int i = 0; i < 3; ++i)
		EXPECT_EQ(i + 1, dest[i]);
}
#endif

TEST_F(dynarrayTest, insertRTrivial)
{
	// Should hit static_assert
	//dynarray<double> d;
	//d.insert_range( d.begin(), oel::view::subrange(std::istream_iterator<double>{}, std::istream_iterator<double>{}) );

	size_t const initSize = 2;
	auto toInsert = {-1.0, -2.0};
	for (auto nReserve : {initSize, initSize + toInsert.size()})
		for (size_t insertOffset = 0; insertOffset <= initSize; ++insertOffset)
		{	{
				dynarrayTrackingAlloc<double> dest(oel::reserve, nReserve);
				dest.emplace_back(1);
				dest.emplace_back(2);

				dest.insert_range(dest.begin() + insertOffset, toInsert);

				EXPECT_TRUE(dest.size() == initSize + toInsert.size());
				for (size_t i = 0; i < toInsert.size(); ++i)
					EXPECT_TRUE( toInsert.begin()[i] == dest[i + insertOffset] );

				if (insertOffset == 0)
				{
					EXPECT_EQ(1, *(dest.end() - 2));
					EXPECT_EQ(2, *(dest.end() - 1));
				}
				else if (insertOffset == initSize)
				{
					EXPECT_EQ(1, dest[0]);
					EXPECT_EQ(2, dest[1]);
				}
				else
				{	EXPECT_EQ(1, dest.front());
					EXPECT_EQ(2, dest.back());
				}
			}
			EXPECT_EQ(g_allocCount.nAllocations, g_allocCount.nDeallocations);
		}
}

TEST_F(dynarrayTest, insertR)
{
	size_t const initSize = 2;
	std::array<TrivialRelocat, 2> const toInsert{TrivialRelocat{-1}, TrivialRelocat{-2}};
	for (auto nReserve : {initSize, initSize + toInsert.size()})
		for (size_t insertOffset = 0; insertOffset <= initSize; ++insertOffset)
			for (unsigned countThrow = 0; countThrow <= toInsert.size(); ++countThrow)
			{	{
					dynarray<TrivialRelocat> dest(oel::reserve, nReserve);
					dest.emplace_back(1);
					dest.emplace_back(2);

					if (countThrow < toInsert.size())
					{
					#if OEL_HAS_EXCEPTIONS
						TrivialRelocat::countToThrowOn = countThrow;
						EXPECT_THROW( dest.insert_range(dest.begin() + insertOffset, toInsert), TestException );
					#endif
						EXPECT_TRUE(initSize <= dest.size() and dest.size() <= initSize + countThrow);
					}
					else
					{	dest.insert_range(dest.begin() + insertOffset, toInsert);

						EXPECT_TRUE(dest.size() == initSize + toInsert.size());
					}
					if (dest.size() > initSize)
					{
						for (unsigned i = 0; i < countThrow; ++i)
							EXPECT_TRUE( *toInsert[i] == *dest[i + insertOffset] );
					}
					if (insertOffset == 0)
					{
						EXPECT_EQ(1, **(dest.end() - 2));
						EXPECT_EQ(2, **(dest.end() - 1));
					}
					else if (insertOffset == initSize)
					{
						EXPECT_EQ(1, *dest[0]);
						EXPECT_EQ(2, *dest[1]);
					}
					else
					{	EXPECT_EQ(1, *dest.front());
						EXPECT_EQ(2, *dest.back());
					}
				}
				EXPECT_EQ(TrivialRelocat::nConstructions, TrivialRelocat::nDestruct + oel::ssize(toInsert));
			}
}

TEST_F(dynarrayTest, emplace)
{
	ptrdiff_t const initSize{2};
	double const firstVal {9};
	double const secondVal{7.5};
	for (auto const nReserve : {initSize, initSize + 1})
		for (ptrdiff_t insertOffset = 0; insertOffset <= initSize; ++insertOffset)
			for (auto const constructThrowOnCount : {0, 1})
				for (auto const allocThrowOnCount : {0, 1})
				{
					{	TrivialRelocat::countToThrowOn = -1;
						g_allocCount.countToThrowOn    = -1;

						dynarrayTrackingAlloc<TrivialRelocat> dest(oel::reserve, nReserve);
						dest.emplace(dest.begin(), firstVal);
						dest.emplace(dest.begin(), secondVal);

						TrivialRelocat::countToThrowOn = constructThrowOnCount;
						g_allocCount.countToThrowOn    = allocThrowOnCount;

						if (constructThrowOnCount == 0 or (allocThrowOnCount == 0 and initSize == nReserve))
						{
						#if OEL_HAS_EXCEPTIONS
							EXPECT_THROW( dest.emplace(dest.begin() + insertOffset), TestException );

							EXPECT_EQ(initSize, ssize(dest));
						#endif
						}
						else
						{	dest.emplace(dest.begin() + insertOffset);

							EXPECT_EQ(initSize + 1, ssize(dest));
							EXPECT_FALSE( dest.at(insertOffset).hasValue() );
						}
						if (insertOffset == 0)
						{
							EXPECT_EQ(firstVal,  **(dest.end() - 1));
							EXPECT_EQ(secondVal, **(dest.end() - 2));
						}
						else if (insertOffset == initSize)
						{
							EXPECT_EQ(secondVal, *dest[0]);
							EXPECT_EQ(firstVal,  *dest[1]);
						}
						else
						{	EXPECT_EQ(secondVal, *dest.front());
							EXPECT_EQ(firstVal,  *dest.back());
						}
					}
					EXPECT_EQ(TrivialRelocat::nConstructions, TrivialRelocat::nDestruct);
					EXPECT_EQ(g_allocCount.nAllocations, g_allocCount.nDeallocations);
				}
}

TEST_F(dynarrayTest, insertTrivialAndCheckReturn)
{
	dynarray<int> test;

	int const values[]{-3, -1, 7, 8};

	auto it = test.insert(begin(test), values[2]);
	EXPECT_EQ(test.data(), &*it);

	it = test.insert(begin(test), values[0]);
	EXPECT_EQ(&test.front(), &*it);

	it = test.insert(end(test), values[3]);
	EXPECT_EQ(&test.back(), &*it);

	it = test.insert(begin(test) + 1, values[1]);
	EXPECT_EQ(&test[1], &*it);

	EXPECT_TRUE( std::equal(std::begin(values), std::end(values), begin(test), end(test)) );
}

TEST_F(dynarrayTest, insertRefFromSelf)
{
	{	dynarrayTrackingAlloc<TrivialRelocat> test;

		test.emplace(begin(test), 7);

		test.insert(begin(test), test.back());

		EXPECT_EQ(7, *test.front());

		test.back() = TrivialRelocat{8};
		test.insert(begin(test), test.back());

		EXPECT_EQ(8, *test.front());
	}
	EXPECT_EQ(TrivialRelocat::nConstructions - 1, g_allocCount.nConstructCalls);
}

TEST_F(dynarrayTest, mutableBeginSizeRange)
{
	int src[1] {1};
	auto v = ToMutableBeginSizeView(src);
	dynarray<int> dest;

	dest.assign(v);
	EXPECT_EQ(1u, dest.size());

	dest.insert_range(dest.begin(), v);
	EXPECT_EQ(2u, dest.size());

	dest.append(v);
	EXPECT_EQ(3u, dest.size());
}

#ifndef NO_VIEWS_ISTREAM

TEST_F(dynarrayTest, moveOnlyIterator)
{
	dynarray<int> dest;
	{
		std::istringstream ss{"1 2 3 4"};
		auto v = std::views::istream<int>(ss);

		auto it = dest.append(view::counted(v.begin(), 3));
		EXPECT_EQ(3u, dest.size());
		EXPECT_EQ(1, dest[0]);
		EXPECT_EQ(2, dest[1]);
		EXPECT_EQ(3, dest[2]);

		it = dest.assign( view::subrange(std::move(it), v.end()) );
		EXPECT_EQ(v.end(), it);
		EXPECT_EQ(1u, dest.size());
		EXPECT_EQ(4, dest[0]);
	}
	std::istringstream ss{"5 6 7 8"};
	auto v = std::views::istream<int>(ss);

	auto it = dest.assign(view::counted(v.begin(), 2));
	EXPECT_EQ(2u, dest.size());
	EXPECT_EQ(5, dest[0]);
	EXPECT_EQ(6, dest[1]);

	dest.insert_range(dest.begin() + 1, view::counted(std::move(it), 2));
	EXPECT_EQ(4u, dest.size());
	EXPECT_EQ(5, dest[0]);
	EXPECT_EQ(7, dest[1]);
	EXPECT_EQ(8, dest[2]);
	EXPECT_EQ(6, dest[3]);
}
#endif

TEST_F(dynarrayTest, resize)
{
	dynarray<int, throwingAlloc<int>> d;

	size_t const S1 = 4;

	d.resize(S1);
	ASSERT_EQ(S1, d.size());

#if OEL_HAS_EXCEPTIONS
	EXPECT_THROW(d.resize_for_overwrite(d.max_size()), std::bad_alloc);
	EXPECT_EQ(S1, d.size());
#endif
	for (const auto & e : d)
	{
		EXPECT_EQ(0, e);
	}


	dynarray< dynarray<int> > nested;
	nested.resize(3);
	EXPECT_EQ(3U, nested.size());
	EXPECT_TRUE(nested.back().empty());

	nested.front().resize(S1);

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

	EXPECT_EQ(S1, nested.front().size());
	EXPECT_TRUE(nested.back().empty());
}

struct NonPowerOfTwo
{
	char data[(sizeof(void *) * 3) / 2];
};

template< typename T = NonPowerOfTwo >
struct StaticBufAlloc
{
	using value_type = T;
	using is_always_equal = std::true_type;

	value_type * buff = nullptr;
	size_t size = 0;

	template<size_t N>
	StaticBufAlloc(value_type (&array)[N])
	 :	buff(array), size(N) {
	}

	template< typename U >
	StaticBufAlloc(const StaticBufAlloc<U> & other)
	 :	buff{other.buff}, size{other.size} {}

	size_t max_size() const { return size; }

	value_type * allocate(size_t n)
	{
		if (n > size)
			oel::_detail::LengthError::raise();

		size = 0;
		return buff;
	}

	void deallocate(value_type *, size_t) {}
};

TEST_F(dynarrayTest, statefulAlwaysEqualDefaultConstructibleAlloc)
{
	struct Mem
	{
		NonPowerOfTwo prefix;
		NonPowerOfTwo use[3];
		NonPowerOfTwo postfix;

		enum : char { testValue = 91 };

		Mem()
		{
			std::fill(std::begin(prefix.data), std::end(prefix.data), testValue);
			std::fill(std::begin(postfix.data), std::end(postfix.data), testValue);
		}

		static bool isValid(const NonPowerOfTwo & padding)
		{
			for (auto e : padding.data)
			{
				if (e != testValue)
					return false;
			}
			return true;
		}
	}
	mem;
	StaticBufAlloc<> a{mem.use};

	dynarray<NonPowerOfTwo, StaticBufAlloc<>> d(a);
	d.resize(d.max_size());

	EXPECT_TRUE(Mem::isValid(mem.prefix));
	EXPECT_TRUE(Mem::isValid(mem.postfix));
}

template<typename T>
void testEraseOne()
{
	dynarray<T> d;

	for (int i = 1; i <= 5; ++i)
		d.emplace_back(i);

	auto const s = d.size();
	auto ret = d.erase(begin(d) + 1);
	ret = d.erase(ret);
	EXPECT_EQ(begin(d) + 1, ret);
	ASSERT_EQ(s - 2, d.size());
	EXPECT_EQ(5, static_cast<double>(d.back()));

	ret = d.erase(end(d) - 1);
	EXPECT_EQ(end(d), ret);
	ASSERT_EQ(s - 3, d.size());
	EXPECT_EQ(1, static_cast<double>(d.front()));
}

TEST_F(dynarrayTest, eraseSingleInt)
{
	testEraseOne<int>();
}

TEST_F(dynarrayTest, eraseSingleTrivialReloc)
{
	testEraseOne<TrivialRelocat>();
}

TEST_F(dynarrayTest, eraseSingle)
{
	testEraseOne<MoveOnly>();
}

template<typename T>
void testErase()
{
	dynarray<T> d;

	for (int i = 1; i <= 5; ++i)
		d.emplace_back(i);

	auto const s = d.size();
	auto ret = d.erase(begin(d) + 2, begin(d) + 2);
	ASSERT_EQ(s, d.size());
	ret = d.erase(ret - 1, ret + 1);
	EXPECT_EQ(begin(d) + 1, ret);
	ASSERT_EQ(s - 2, d.size());
	EXPECT_EQ(s, static_cast<double>(d.back()));
}

TEST_F(dynarrayTest, eraseRangeInt)
{
	testErase<int>();
}

TEST_F(dynarrayTest, eraseRangeTrivialReloc)
{
	testErase<TrivialRelocat>();
}

TEST_F(dynarrayTest, eraseRange)
{
	testErase<MoveOnly>();
}

TEST_F(dynarrayTest, eraseToEnd)
{
	oel::dynarray<int> li{1, 1, 2, 2, 2, 1, 3};
	li.erase_to_end(std::remove(begin(li), end(li), 1));
	EXPECT_EQ(4U, li.size());
}

#if OEL_MEM_BOUND_DEBUG_LVL
TEST_F(dynarrayTest, erasePrecondCheck)
{
	leakDetector->enabled = false;

	dynarray<int> di{-2};
	auto copy = di;
	ASSERT_DEATH( copy.erase(di.begin()), "" );
}

TEST_F(dynarrayTest, unorderErasePrecondCheck)
{
	dynarray<int> di{1};
	ASSERT_DEATH( di.unordered_erase(di.end()), "" );
}
#endif

template< typename T >
void testUnorderedErase()
{
	dynarray<T> d;
	d.emplace_back(1);
	d.emplace_back(-2);

	auto it = d.unordered_erase(d.begin());
	EXPECT_EQ(1U, d.size());
	EXPECT_EQ(-2, *(*it));
	it = d.unordered_erase(it);
	EXPECT_EQ(end(d), it);

	d.emplace_back(-1);
	d.emplace_back(2);
	unordered_erase(d, 1);
	EXPECT_EQ(-1, *d.back());
	unordered_erase(d, 0);
	EXPECT_TRUE(d.empty());
}

TEST_F(dynarrayTest, unorderedErase)
{
	testUnorderedErase<MoveOnly>();
}

TEST_F(dynarrayTest, unorderedEraseTrivialReloc)
{
	testUnorderedErase<TrivialRelocat>();
}

TEST_F(dynarrayTest, shrinkToFit)
{
	dynarray<MoveOnly> d(oel::reserve, 9);
	d.emplace_back(-5);

	d.shrink_to_fit();

	EXPECT_GT(9u, d.capacity());
	EXPECT_EQ(1u, d.size());
}

TEST_F(dynarrayTest, overAligned)
{
	constexpr auto testAlignment = OEL_MALLOC_ALIGNMENT * 2;
	struct alignas(testAlignment) Type
	{
		double v[2];
	};
	dynarray<Type> special(oel::reserve, 1);

	special.insert(special.begin(), Type());
	EXPECT_EQ(0U, reinterpret_cast<std::uintptr_t>(special.data()) % testAlignment);

	special.emplace(special.begin());
	special.emplace(special.begin() + 1);
	EXPECT_EQ(3U, special.size());
	for (const auto & v : special)
		EXPECT_EQ(0U, reinterpret_cast<std::uintptr_t>(&v) % testAlignment);

	special.unordered_erase(special.end() - 1);
	special.unordered_erase(special.begin());
	special.shrink_to_fit();
	EXPECT_TRUE(special.capacity() < 3U);
	EXPECT_EQ(0U, reinterpret_cast<std::uintptr_t>(&special.front()) % testAlignment);

#if OEL_HAS_EXCEPTIONS
	EXPECT_THROW(special.reserve(special.max_size()),     std::bad_alloc);
	EXPECT_THROW(special.reserve(special.max_size() - 1), std::bad_alloc);
	EXPECT_THROW(special.reserve(special.max_size() + 1), std::length_error);
#endif
}

#if OEL_HAS_EXCEPTIONS
TEST_F(dynarrayTest, greaterThanMax)
{
	struct Size2
	{
		char a[2];
	};

	dynarray<Size2> d;
	auto const n = std::numeric_limits<size_t>::max() / 2 + 1;

	EXPECT_THROW(d.reserve(SIZE_MAX), std::length_error);
	EXPECT_THROW(d.reserve(n), std::length_error);
	EXPECT_THROW(d.resize(n), std::length_error);
	EXPECT_THROW(d.resize_for_overwrite(n), std::length_error);
	ASSERT_TRUE(d.empty());
	EXPECT_THROW(d.append(n, Size2{{}}), std::length_error);
}
#endif

TEST_F(dynarrayTest, noDefaultConstructAlloc)
{
	dynarray<int, noDefaultConstructAlloc<>> test(noDefaultConstructAlloc<>(0));
	test.push_back(1);
	EXPECT_EQ(1u, test.size());
}

TEST_F(dynarrayTest, misc)
{
	size_t fASrc[] = { 2, 3 };

	dynarray<size_t> daSrc(oel::reserve, 2);
	daSrc.push_back(0);
	daSrc.push_back(2);
	daSrc.insert(begin(daSrc) + 1, 1);
	ASSERT_EQ(3U, daSrc.size());

#if OEL_HAS_EXCEPTIONS
	ASSERT_NO_THROW(daSrc.at(2));
	ASSERT_THROW(daSrc.at(3), std::out_of_range);
#endif
	std::deque<size_t> dequeSrc{4, 5};

	dynarray<size_t> dest0;
	dest0.reserve(1);
	dest0 = daSrc;

	dest0.append( view::counted(daSrc.cbegin(), daSrc.size()) );
	dest0.append(view::counted(fASrc, 2));
	auto srcEnd = dest0.append( view::counted(begin(dequeSrc), dequeSrc.size()) );
	EXPECT_TRUE(end(dequeSrc) == srcEnd);

	dynarray<size_t> dest1;
	dest1.append(daSrc);
	dest1.append(fASrc);
	dest1.append(dequeSrc);

	auto cap = dest1.capacity();
	dest1.pop_back();
	dest1.pop_back();
	dest1.shrink_to_fit();
	EXPECT_GT(cap, dest1.capacity());
}

#if OEL_STD_RANGES
void testDanglingReturn()
{
	dynarray<int> d;
	auto i0 = d.assign(dynarray<int>{});
	auto i1 = d.append(dynarray<int>{});
	static_assert(std::is_same_v<decltype(i0), std::ranges::dangling>);
	static_assert(std::is_same_v<decltype(i1), std::ranges::dangling>);
}
#endif
