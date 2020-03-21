// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "throw_from_assert.h"
#include "test_classes.h"

#include "range_view.h"
#include "dynarray.h"

#include <deque>
#include <array>


using oel::dynarray;
namespace view = oel::view;

struct noDefaultConstructAlloc : public oel::allocator<int>
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
static_assert( !oel::is_always_equal<throwingAlloc<int>>::value, "?" );

// The fixture for testing dynarray.
class dynarrayTest : public ::testing::Test
{
protected:
	dynarrayTest()
	{
		AllocCounter::ClearAll();
		MyCounter::ClearCount();
	}

	// Objects declared here can be used by all tests.
};

TEST_F(dynarrayTest, pushBack)
{
	{
		dynarray<MoveOnly> up;

		double const VALUES[] = {-1.1, 2.0};

		up.push_back(MoveOnly{VALUES[0]});
		ASSERT_EQ(1U, up.size());

	OEL_WHEN_EXCEPTIONS_ON(
		MoveOnly::countToThrowOn = 0;
		EXPECT_THROW( up.emplace_back(), TestException );
		ASSERT_EQ(1U, up.size());
	)
		up.push_back(MoveOnly{VALUES[1]});
		ASSERT_EQ(2U, up.size());

	OEL_WHEN_EXCEPTIONS_ON(
		MoveOnly::countToThrowOn = 0;
		EXPECT_THROW( up.emplace_back(), TestException );
		ASSERT_EQ(2U, up.size());
	)
		up.push_back( std::move(up.back()) );
		ASSERT_EQ(3U, up.size());

		EXPECT_EQ(VALUES[0], *up[0]);
		EXPECT_EQ(nullptr, up[1].get());
		EXPECT_EQ(VALUES[1], *up[2]);
	}
	EXPECT_EQ(MoveOnly::nConstructions, MoveOnly::nDestruct);

	dynarray< dynarray<int> > nested;
	nested.emplace_back(3, oel::default_init);
	EXPECT_EQ(3U, nested.back().size());
	nested.emplace_back(std::initializer_list<int>{1, 2});
	EXPECT_EQ(2U, nested.back().size());
}

TEST_F(dynarrayTest, pushBackNonTrivialReloc)
{
	{
		dynarray<TrivialRelocat> da;

		double const VALUES[] = {-1.1, 2.0, -0.7, 9.6};
		std::deque<double> expected;

		da.push_back(TrivialRelocat{VALUES[0]});
		expected.push_back(VALUES[0]);
		ASSERT_EQ(1U, da.size());
		EXPECT_EQ(TrivialRelocat::nConstructions - ssize(da), TrivialRelocat::nDestruct);

		da.emplace_back(VALUES[1]);
		expected.emplace_back(VALUES[1]);
		ASSERT_EQ(2U, da.size());
		EXPECT_EQ(TrivialRelocat::nConstructions - ssize(da), TrivialRelocat::nDestruct);

	OEL_WHEN_EXCEPTIONS_ON(
		TrivialRelocat::countToThrowOn = 1;
		try
		{
			for(;;)
			{
				da.push_back(TrivialRelocat{VALUES[2]});
				expected.push_back(VALUES[2]);
			}
		}
		catch (TestException &) {
		}
		ASSERT_EQ(expected.size(), da.size());
		EXPECT_EQ(TrivialRelocat::nConstructions - ssize(da), TrivialRelocat::nDestruct);
	)
		da.emplace_back(VALUES[3]);
		expected.emplace_back(VALUES[3]);
		ASSERT_EQ(expected.size(), da.size());

	OEL_WHEN_EXCEPTIONS_ON(
		TrivialRelocat::countToThrowOn = 0;
		EXPECT_THROW( da.push_back(TrivialRelocat{0}), TestException );
		ASSERT_EQ(expected.size(), da.size());
	)
		EXPECT_EQ(TrivialRelocat::nConstructions - ssize(da), TrivialRelocat::nDestruct);

	OEL_WHEN_EXCEPTIONS_ON(
		TrivialRelocat::countToThrowOn = 3;
		try
		{
			for(;;)
			{
				da.push_back(da.front());
				expected.push_back(expected.front());
			}
		}
		catch (TestException &) {
		}
		ASSERT_EQ(expected.size(), da.size());
	)
		EXPECT_TRUE( std::equal(begin(da), end(da), begin(expected)) );
	}
	EXPECT_EQ(TrivialRelocat::nConstructions, TrivialRelocat::nDestruct);
}

TEST_F(dynarrayTest, assign)
{
	MoveOnly::ClearCount();
	{
		double const VALUES[] = {-1.1, 0.4};
		MoveOnly src[] { MoveOnly{VALUES[0]},
						 MoveOnly{VALUES[1]} };
		dynarray<MoveOnly> test;

		test.assign(view::move(src));

		EXPECT_EQ(2U, test.size());
		EXPECT_EQ(VALUES[0], *test[0]);
		EXPECT_EQ(VALUES[1], *test[1]);

		test.assign(view::move(src, src));
		EXPECT_EQ(0U, test.size());
	}
	EXPECT_EQ(MoveOnly::nConstructions, MoveOnly::nDestruct);

	TrivialRelocat::ClearCount();
	{
		dynarray<TrivialRelocat> dest;
		OEL_WHEN_EXCEPTIONS_ON(
		{
			TrivialRelocat obj{0};
			TrivialRelocat::countToThrowOn = 0;
			EXPECT_THROW(
				dest.assign(view::counted(&obj, 1)),
				TestException );
			EXPECT_TRUE(dest.begin() == dest.end());
		} )
		EXPECT_EQ(TrivialRelocat::nConstructions, TrivialRelocat::nDestruct);

		dest = {TrivialRelocat{-1.0}};
		EXPECT_EQ(1U, dest.size());
		dest = {TrivialRelocat{1.0}, TrivialRelocat{2.0}};
		EXPECT_EQ(1.0, *dest.at(0));
		EXPECT_EQ(2.0, *dest.at(1));
		EXPECT_EQ(TrivialRelocat::nConstructions - ssize(dest), TrivialRelocat::nDestruct);
		OEL_WHEN_EXCEPTIONS_ON(
		{
			TrivialRelocat obj{0};
			TrivialRelocat::countToThrowOn = 0;
			EXPECT_THROW(
				dest.assign(view::subrange(&obj, &obj + 1)),
				TestException );
			EXPECT_TRUE(dest.empty() or *dest.at(1) == 2.0);
		} )
		{
			dest.clear();
			EXPECT_LE(2U, dest.capacity());
			EXPECT_TRUE(dest.empty());

		OEL_WHEN_EXCEPTIONS_ON(
			TrivialRelocat obj{0};
			TrivialRelocat::countToThrowOn = 0;
			EXPECT_THROW(
				dest.assign(view::counted(&obj, 1)),
				TestException );
			EXPECT_TRUE(dest.empty());
		)
		}
	}
	EXPECT_EQ(TrivialRelocat::nConstructions, TrivialRelocat::nDestruct);
}

// std::stringstream doesn't seem to work using libstdc++ with -fno-exceptions
#if !defined __GLIBCXX__ or defined __EXCEPTIONS
TEST_F(dynarrayTest, assignNonForwardRange)
{
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
	ASSERT_EQ(AllocCounter::nAllocations, AllocCounter::nDeallocations);
}
#endif

TEST_F(dynarrayTest, append)
{
	{
		oel::dynarray<double> dest;
		// Test append empty std iterator range to empty dynarray
		std::deque<double> src;
		dest.append(src);

		dest.append({});
		EXPECT_EQ(0U, dest.size());

		double const TEST_VAL = 6.6;
		dest.append(2, TEST_VAL);
		dest.append( view::subrange(dest.begin(), dest.end()) );
		EXPECT_EQ(4U, dest.size());
		for (const auto & d : dest)
			EXPECT_EQ(TEST_VAL, d);
	}

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

#if defined _CPPUNWIND or defined __EXCEPTIONS
TEST_F(dynarrayTest, appendSizeOverflow)
{
	dynarray<char> c(1);
	try
	{	c.append((size_t)-1, '\0');
		EXPECT_TRUE(false);
	}
	catch (std::bad_alloc &)
	{}
	catch (std::length_error &)
	{}
}
#endif

#if !defined __GLIBCXX__ or defined __EXCEPTIONS
TEST_F(dynarrayTest, appendNonForwardRange)
{
	{
		std::stringstream ss("1 2 3 4 5");

		dynarrayTrackingAlloc<int> dest;

		std::istream_iterator<int> it(ss), end{};

		it = dest.append(view::counted(it, 2));

		dest.append(view::subrange(it, end));

		for (int i = 0; i < 5; ++i)
			EXPECT_EQ(i + 1, dest[i]);
	}
	ASSERT_EQ(AllocCounter::nAllocations, AllocCounter::nDeallocations);
}
#endif

TEST_F(dynarrayTest, insertRTrivial)
{
	// Should hit static_assert
	//dynarray<double> d;
	//d.insert_r( d.begin(), oel::iterator_range< std::istream_iterator<double> >({}, {}) );

	size_t const initSize = 2;
	auto toInsert = {-1.0, -2.0};
	for (auto nReserve : {initSize, initSize + toInsert.size()})
		for (size_t insertOffset = 0; insertOffset <= initSize; ++insertOffset)
		{	{
				dynarrayTrackingAlloc<double> dest(oel::reserve, nReserve);
				dest.emplace_back(1);
				dest.emplace_back(2);

				dest.insert(dest.begin() + insertOffset, toInsert);

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
			EXPECT_EQ(AllocCounter::nAllocations, AllocCounter::nDeallocations);
		}
}

TEST_F(dynarrayTest, insertR)
{
	using oel::ssize;

	size_t const initSize = 2;
	std::array<TrivialRelocat, 2> const toInsert{TrivialRelocat{-1}, TrivialRelocat{-2}};
	for (auto nReserve : {initSize, initSize + toInsert.size()})
		for (size_t insertOffset = 0; insertOffset <= initSize; ++insertOffset)
			for (int countThrow = 0; countThrow <= ssize(toInsert); ++countThrow)
			{	{
					dynarray<TrivialRelocat> dest(oel::reserve, nReserve);
					dest.emplace_back(1);
					dest.emplace_back(2);

					if (countThrow < ssize(toInsert))
					{
					OEL_WHEN_EXCEPTIONS_ON(
						TrivialRelocat::countToThrowOn = countThrow;
						EXPECT_THROW( dest.insert_r(dest.begin() + insertOffset, toInsert), TestException );
					)
						EXPECT_TRUE(initSize <= dest.size() and dest.size() <= initSize + countThrow);
					}
					else
					{	dest.insert_r(dest.begin() + insertOffset, toInsert);

						EXPECT_TRUE(dest.size() == initSize + toInsert.size());
					}
					if (dest.size() > initSize)
					{
						for (int i = 0; i < countThrow; ++i)
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
				EXPECT_EQ(TrivialRelocat::nConstructions, TrivialRelocat::nDestruct + ssize(toInsert));
			}
}

TEST_F(dynarrayTest, insert)
{
	{
		dynarray<TrivialRelocat> test;

		double const VALUES[] = {-1.1, 0.4, 1.3, 2.2};

		auto & ptr = *test.emplace(begin(test), VALUES[2]);
		EXPECT_EQ(VALUES[2], *ptr);
		ASSERT_EQ(1U, test.size());

	OEL_WHEN_EXCEPTIONS_ON(
		TrivialRelocat::countToThrowOn = 0;
		EXPECT_THROW( test.insert(begin(test), TrivialRelocat{0.0}), TestException );
		ASSERT_EQ(1U, test.size());
	)
		test.insert(begin(test), TrivialRelocat{VALUES[0]});
		ASSERT_EQ(2U, test.size());

	OEL_WHEN_EXCEPTIONS_ON(
		TrivialRelocat::countToThrowOn = 0;
		EXPECT_THROW( test.insert(begin(test) + 1, TrivialRelocat{0.0}), TestException );
		ASSERT_EQ(2U, test.size());
	)
		test.emplace(end(test), VALUES[3]);
		auto & p2 = *test.insert(begin(test) + 1, TrivialRelocat{VALUES[1]});
		EXPECT_EQ(VALUES[1], *p2);
		ASSERT_EQ(4U, test.size());

		auto v = std::begin(VALUES);
		for (const auto & p : test)
		{
			EXPECT_EQ(*v, *p);
			++v;
		}

		auto it = test.insert( begin(test) + 2, std::move(test[2]) );
		EXPECT_EQ(test[2].get(), it->get());

		auto const val = *test.back();
		test.insert( end(test) - 1, std::move(test.back()) );
		ASSERT_EQ(6U, test.size());
		EXPECT_EQ(val, *end(test)[-2]);
	}
	EXPECT_EQ(TrivialRelocat::nConstructions, TrivialRelocat::nDestruct);
}

TEST_F(dynarrayTest, resize)
{
	dynarray<int, throwingAlloc<int>> d;

	size_t const S1 = 4;

	d.resize(S1);
	ASSERT_EQ(S1, d.size());

OEL_WHEN_EXCEPTIONS_ON(
	EXPECT_THROW(d.resize_default_init(d.max_size()), std::bad_alloc);
	EXPECT_EQ(S1, d.size());
)
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

struct StaticBufAlloc
{
	using value_type = NonPowerOfTwo;
	using is_always_equal = std::true_type;

	value_type * buf = nullptr;
	size_t size = 0;

	StaticBufAlloc() = default;

	template<size_t N>
	StaticBufAlloc(value_type (&array)[N])
	 :	buf(array), size(N) {
	}

	size_t max_size() const { return size; }

	value_type * allocate(size_t n)
	{
		if (n > size)
			oel::_detail::Throw::LengthError("StaticBufAlloc::allocate n > size");

		size = 0;
		return buf;
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

		static bool IsValid(const NonPowerOfTwo & padding)
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
	StaticBufAlloc a{mem.use};

	dynarray<NonPowerOfTwo, StaticBufAlloc> d(a);
	d.resize(d.max_size());

	EXPECT_TRUE(Mem::IsValid(mem.prefix));
	EXPECT_TRUE(Mem::IsValid(mem.postfix));
}

template<typename T>
void testErase()
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

TEST_F(dynarrayTest, eraseSingle)
{
	testErase<int>();

	MoveOnly::ClearCount();
	testErase<MoveOnly>();
	EXPECT_EQ(MoveOnly::nConstructions, MoveOnly::nDestruct);
}

TEST_F(dynarrayTest, eraseRange)
{
	dynarray<unsigned int> d;

	for (int i = 1; i <= 5; ++i)
		d.push_back(i);

	auto const s = d.size();
	auto ret = d.erase(begin(d) + 2, begin(d) + 2);
	ASSERT_EQ(s, d.size());
	ret = d.erase(ret - 1, ret + 1);
	EXPECT_EQ(begin(d) + 1, ret);
	ASSERT_EQ(s - 2, d.size());
	EXPECT_EQ(s, d.back());
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
	dynarray<int> di{-2};

	OEL_WHEN_EXCEPTIONS_ON(
		EXPECT_THROW(di.erase_unstable(di.end()), std::logic_error);
	)
	OEL_WHEN_EXCEPTIONS_ON(
		auto copy = di;
		EXPECT_THROW(copy.erase(di.begin()), std::logic_error);
	)
}
#endif

TEST_F(dynarrayTest, eraseUnstable)
{
	{
		dynarray<MoveOnly> d;
		d.emplace_back(1);
		d.emplace_back(-2);

		auto *const p = d.back().get();
		auto it = d.erase_unstable(d.begin());
		EXPECT_EQ(p, it->get());
		EXPECT_EQ(-2, *(*it));
		it = d.erase_unstable(it);
		EXPECT_EQ(end(d), it);

		d.emplace_back(-1);
		d.emplace_back(2);
		erase_unstable(d, 1);
		EXPECT_EQ(-1, *d.back());
		erase_unstable(d, 0);
		EXPECT_TRUE(d.empty());
	}
	EXPECT_EQ(MoveOnly::nConstructions, MoveOnly::nDestruct);
}

TEST_F(dynarrayTest, overAligned)
{
	static unsigned int const testAlignment = 64;
	struct Type
	{	oel::aligned_storage_t<testAlignment, testAlignment> a;
	};
	dynarray<Type> special(oel::reserve, 1);

	special.insert(special.begin(), Type());
	EXPECT_EQ(0U, reinterpret_cast<std::uintptr_t>(special.data()) % testAlignment);

	special.emplace(special.begin());
	special.emplace(special.begin() + 1);
	EXPECT_EQ(3U, special.size());
	for (const auto & v : special)
		EXPECT_EQ(0U, reinterpret_cast<std::uintptr_t>(&v) % testAlignment);

	special.erase_unstable(special.end() - 1);
	special.erase_unstable(special.begin());
	special.shrink_to_fit();
	EXPECT_TRUE(special.capacity() < 3U);
	EXPECT_EQ(0U, reinterpret_cast<std::uintptr_t>(&special.front()) % testAlignment);

OEL_WHEN_EXCEPTIONS_ON(
	try
	{	special.reserve(special.max_size());
		EXPECT_FALSE(true);
	}
	catch(const std::bad_alloc &) {}
	catch(const std::length_error &) {}
)
OEL_WHEN_EXCEPTIONS_ON(
	EXPECT_ANY_THROW( special.reserve(special.max_size() - 1) );
)
}

#if defined _CPPUNWIND or defined __EXCEPTIONS
TEST_F(dynarrayTest, greaterThanMax)
{
	struct Size2
	{
		char a[2];
	};

	dynarray<Size2> d;
	size_t const n = std::numeric_limits<size_t>::max() / 2 + 1;

	EXPECT_THROW(d.reserve((size_t) -1), std::length_error);
	EXPECT_THROW(d.reserve(n), std::length_error);
	EXPECT_THROW(d.resize(n), std::length_error);
	EXPECT_THROW(d.resize_default_init(n), std::length_error);
	ASSERT_TRUE(d.empty());
	EXPECT_THROW(d.append(n, Size2{{}}), std::length_error);
}
#endif

TEST_F(dynarrayTest, misc)
{
	size_t fASrc[] = { 2, 3 };

	dynarray<size_t> daSrc(oel::reserve, 2);
	daSrc.push_back(0);
	daSrc.push_back(2);
	daSrc.insert(begin(daSrc) + 1, 1);
	ASSERT_EQ(3U, daSrc.size());

OEL_WHEN_EXCEPTIONS_ON(
	ASSERT_NO_THROW(daSrc.at(2));
	ASSERT_THROW(daSrc.at(3), std::out_of_range);
)
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

	{
		dynarray<int, noDefaultConstructAlloc> test(noDefaultConstructAlloc(0));
		test.push_back(1);
	}
}
