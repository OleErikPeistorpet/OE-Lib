#include "generic_array_test.h"
#include "inplace_growarr.h"
#include "range_algo.h"

using oel::inplace_growarr;

using FcaString   = inplace_growarr<std::string, 1>;
using FcaMayThrow = inplace_growarr<NontrivialConstruct, 1>;

static_assert( std::is_nothrow_default_constructible<FcaMayThrow>{});
static_assert(    std::is_nothrow_move_constructible<FcaString>{});
static_assert(not std::is_nothrow_move_assignable<FcaMayThrow>{});

static_assert(not std::is_trivially_copyable_v< FcaString >);

// The fixture for testing inplace_growarr.
class inplaceGrowarrTest : public ::testing::Test
{
protected:
	inplaceGrowarrTest()
	{
	}
};

TEST_F(inplaceGrowarrTest, construct)
{
	//inplace_growarr<int, 0> compileWarnOrFail;

	using Internal = std::deque<double *>;
	inplace_growarr<Internal, 2> test{Internal(5), Internal()};
	EXPECT_EQ(2U, test.size());

	testConstruct< inplace_growarr<std::string, 1>, inplace_growarr<char, 4>, inplace_growarr<bool, 50> >();
}

TEST_F(inplaceGrowarrTest, toInplaceGrowarr)
{
	char from[4]{};
	auto res  = oel::to_inplace_growarr<4>(from);
	auto res2 = from | oel::to_inplace_growarr<7, std::uint_fast8_t>;
	static_assert(std::is_same_v< decltype(res),  inplace_growarr<char, 4> >);
	static_assert(std::is_same_v< decltype(res2), inplace_growarr<char, 7, std::uint_fast8_t> >);
}

TEST_F(inplaceGrowarrTest, pushBackMoveOnly)
{
	testPushBack< inplace_growarr<MoveOnly, 5>, inplace_growarr<inplace_growarr<int, 3>, 2> >();
}

TEST_F(inplaceGrowarrTest, pushBackTrivialReloc)
{
	testPushBackTrivialReloc< inplace_growarr<TrivialRelocat, 7> >();
}

TEST_F(inplaceGrowarrTest, assign)
{
	testAssign< inplace_growarr<MoveOnly, 5>, inplace_growarr<TrivialRelocat, 5> >();
}

TEST_F(inplaceGrowarrTest, assignStringStream)
{
	testAssignStringStream< inplace_growarr<std::string, 5> >();
}

TEST_F(inplaceGrowarrTest, append)
{
	testAppend<	inplace_growarr<double, 8>, inplace_growarr<int, 4> >();
}

TEST_F(inplaceGrowarrTest, appendFromStringStream)
{
	testAppendFromStringStream<	inplace_growarr<int, 5> >();
}

TEST_F(inplaceGrowarrTest, insertRange)
{
	testInsertRange< inplace_growarr<double, 8>, inplace_growarr<int, 4> >();
}

template< ptrdiff_t Cap >
void testEmplace()
{
	TrivialRelocat::clearCount();

	constexpr ptrdiff_t initSize{2};
	double const firstVal {9};
	double const secondVal{7.5};
	for (ptrdiff_t insertOffset = 0; insertOffset <= initSize; ++insertOffset)
		for (auto const constructThrowOnCount : {0, 1})
		{
			{	TrivialRelocat::countToThrowOn = -1;

				inplace_growarr<TrivialRelocat, Cap> dest;
				dest.emplace(dest.begin(), firstVal);
				dest.emplace(dest.begin(), secondVal);

				TrivialRelocat::countToThrowOn = constructThrowOnCount;

				if constexpr (Cap == initSize)
				{
				#if OEL_HAS_EXCEPTIONS
					EXPECT_THROW( dest.emplace(dest.begin() + insertOffset), std::bad_alloc );

					EXPECT_EQ(initSize, ssize(dest));
				#endif
				}
				else if (constructThrowOnCount == 0)
				{
				#if OEL_HAS_EXCEPTIONS
					EXPECT_THROW( dest.emplace(dest.begin() + insertOffset), TestException );

					EXPECT_EQ(initSize, ssize(dest));
				#endif
				}
				else
				{	dest.emplace(dest.begin() + insertOffset);

					EXPECT_EQ(initSize + 1, ssize(dest));
					EXPECT_FALSE( dest[insertOffset].hasValue() );
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
		}
}

TEST_F(inplaceGrowarrTest, emplace)
{
	testEmplace<2>();
	testEmplace<3>();
}

TEST_F(inplaceGrowarrTest, resize)
{
	inplace_growarr<int, 4> d;

	size_t const S1 = 4;

	d.resize(S1);
	ASSERT_EQ(S1, d.size());

#if OEL_HAS_EXCEPTIONS
	EXPECT_THROW(d.resize_for_overwrite(d.max_size() + 1), std::bad_alloc);
	EXPECT_EQ(S1, d.size());
#endif
	for (const auto & e : d)
	{
		EXPECT_EQ(0, e);
	}


	inplace_growarr< inplace_growarr<int, 4>, 4 > nested;
	nested.resize(3);
	EXPECT_EQ(3U, nested.size());
	EXPECT_TRUE(nested.back().empty());

	nested.front().resize(S1);

	nested.resize(1);
	EXPECT_EQ(1U, nested.size());
	for (auto i : nested.front())
		EXPECT_EQ(0, i);

	auto it = nested.begin();

	nested.resize(nested.max_size());

	EXPECT_TRUE(nested.begin() == it);

	EXPECT_EQ(S1, nested.front().size());
	EXPECT_TRUE(nested.back().empty());
}

TEST_F(inplaceGrowarrTest, eraseSingle)
{
	testEraseSingle< inplace_growarr<int, 5>, inplace_growarr<MoveOnly, 5> >();
}

TEST_F(inplaceGrowarrTest, eraseRange)
{
	testEraseRange< inplace_growarr<unsigned, 5> >();
}

TEST_F(inplaceGrowarrTest, eraseToEnd)
{
	testEraseToEnd< inplace_growarr<int, 7> >();
}

TEST_F(inplaceGrowarrTest, overAligned)
{
	constexpr size_t testAlignment = 32;
	struct Type
	{	alignas(testAlignment) unsigned char a[testAlignment];
	};
	inplace_growarr<Type, 5> special(0);
	EXPECT_TRUE(special.cbegin() == special.cend());

	special.insert(special.begin(), Type());
	special.emplace(special.begin());
	special.emplace(special.begin() + 1);
	EXPECT_EQ(3U, special.size());
	for (const auto & v : special)
		EXPECT_EQ(0U, reinterpret_cast<std::uintptr_t>(&v) % testAlignment);

	special.unordered_erase(special.end() - 1);
	special.unordered_erase(special.begin());
	EXPECT_EQ(0U, reinterpret_cast<std::uintptr_t>(&special.front()) % testAlignment);
}

TEST_F(inplaceGrowarrTest, misc)
{
	size_t fASrc[] = { 2, 3 };

	using FCArray3 = inplace_growarr<size_t, 3>;
	using FCArray7 = inplace_growarr<size_t, 7>;

	FCArray3 daSrc({});
	daSrc.push_back(0);
	daSrc.push_back(2);
	daSrc.insert(begin(daSrc) + 1, 1);
	ASSERT_EQ(3U, daSrc.size());

	std::deque<size_t> dequeSrc;
	dequeSrc.push_back(4);
	dequeSrc.push_back(5);

	FCArray7 dest0;
	dest0.try_assign(daSrc);

	dest0.append( view::subrange(daSrc.cbegin(), daSrc.cend() - 1) );
	dest0.pop_back();
	dest0.append(view::counted(fASrc, 2));
	dest0.pop_back();
	auto srcEnd = dest0.append( view::counted(dequeSrc.begin(), dequeSrc.size()) );
	EXPECT_TRUE(end(dequeSrc) == srcEnd);

	FCArray7 dest1;
	FCArray7::const_iterator( dest1.append(daSrc) );
	dest1.append(fASrc);
	dest1.append(dequeSrc);
	EXPECT_EQ(FCArray7::max_size(), dest1.size());

	{
		inplace_growarr<int, 2> di{1, -2};
		auto it = begin(di);
		di.unordered_erase(it);
		EXPECT_EQ(-2, *it);
		di.unordered_erase(it);
		EXPECT_EQ(end(di), it);

		di.assign(2, -1);
		unordered_erase(di, 1);
		unordered_erase(di, 0);
		EXPECT_TRUE(di.empty());
	}
}
