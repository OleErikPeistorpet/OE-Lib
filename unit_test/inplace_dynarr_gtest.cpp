#include "generic_array_test.h"
#include "inplace_dynarr.h"
#include "range_algo.h"

using oel::inplace_dynarr;

using FcaString   = inplace_dynarr<std::string, 1>;
using FcaMayThrow = inplace_dynarr<NontrivialConstruct, 1>;

static_assert( std::is_nothrow_default_constructible<FcaMayThrow>{}, "?");
static_assert(    std::is_nothrow_move_constructible<FcaString>{}, "?");
static_assert(not std::is_nothrow_move_constructible<FcaMayThrow>{}, "?");
static_assert(not std::is_nothrow_move_assignable<FcaMayThrow>{}, "?");

static_assert(    oel::is_trivially_copyable< inplace_dynarr<int, 1> >{}, "?");
static_assert(not oel::is_trivially_copyable< FcaString >{}, "?");

// The fixture for testing inplace_dynarr.
class inplace_dynarrTest : public ::testing::Test
{
protected:
	inplace_dynarrTest()
	{
		// You can do set-up work for each test here.
	}

	// Objects declared here can be used by all tests.
};

TEST_F(inplace_dynarrTest, construct)
{
	//inplace_dynarr<int, 0> compileWarnOrFail;

	using Internal = std::deque<double *>;
	inplace_dynarr<Internal, 2> test{Internal(5), Internal()};
	EXPECT_EQ(2U, test.size());

	testConstruct< inplace_dynarr<std::string, 1>, inplace_dynarr<char, 4>, inplace_dynarr<bool, 50> >();
}

TEST_F(inplace_dynarrTest, pushBackMoveOnly)
{
	testPushBack< inplace_dynarr<MoveOnly, 5>, inplace_dynarr<inplace_dynarr<int, 3>, 2> >();
}

TEST_F(inplace_dynarrTest, pushBackTrivialReloc)
{
	testPushBackTrivialReloc< inplace_dynarr<TrivialRelocat, 5> >();
}

TEST_F(inplace_dynarrTest, assign)
{
	testAssign< inplace_dynarr<MoveOnly, 5>, inplace_dynarr<TrivialRelocat, 5> >();
}

TEST_F(inplace_dynarrTest, assignStringStream)
{
	testAssignStringStream< inplace_dynarr<std::string, 5> >();
}

TEST_F(inplace_dynarrTest, append)
{
	testAppend<	inplace_dynarr<double, 8>, inplace_dynarr<int, 4> >();
}

TEST_F(inplace_dynarrTest, appendFromStringStream)
{
	testAppendFromStringStream<	inplace_dynarr<int, 5> >();
}

TEST_F(inplace_dynarrTest, insertR)
{
	testInsertR< inplace_dynarr<double, 8>, inplace_dynarr<int, 4> >();
}

TEST_F(inplace_dynarrTest, insert)
{
	testInsert< inplace_dynarr<TrivialRelocat, 6> >();
}

TEST_F(inplace_dynarrTest, resize)
{
	inplace_dynarr<int, 4> d;

	size_t const S1 = 4;

	d.resize(S1);
	ASSERT_EQ(S1, d.size());

OEL_WHEN_EXCEPTIONS_ON(
	EXPECT_THROW(d.resize_for_overwrite(d.max_size() + 1), std::length_error);
	EXPECT_EQ(S1, d.size());
)
	for (const auto & e : d)
	{
		EXPECT_EQ(0, e);
	}


	inplace_dynarr< inplace_dynarr<int, 4>, 4 > nested;
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

TEST_F(inplace_dynarrTest, eraseSingle)
{
	testEraseSingle< inplace_dynarr<int, 5>, inplace_dynarr<MoveOnly, 5> >();
}

TEST_F(inplace_dynarrTest, eraseRange)
{
	testEraseRange< inplace_dynarr<unsigned, 5> >();
}

TEST_F(inplace_dynarrTest, eraseToEnd)
{
	testEraseToEnd< inplace_dynarr<int, 7> >();
}

TEST_F(inplace_dynarrTest, overAligned)
{
	unsigned int const testAlignment = 32;
	struct Type
	{	oel::aligned_storage_t<testAlignment, testAlignment> a;
	};
	inplace_dynarr<Type, 5> special(0);
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

TEST_F(inplace_dynarrTest, misc)
{
	size_t fASrc[] = { 2, 3 };

	using FCArray3 = inplace_dynarr<size_t, 3>;
	using FCArray7 = inplace_dynarr<size_t, 7>;

	FCArray3 daSrc({});
	daSrc.push_back(0);
	daSrc.push_back(2);
	daSrc.insert(begin(daSrc) + 1, 1);
	ASSERT_EQ(3U, daSrc.size());

	std::deque<size_t> dequeSrc;
	dequeSrc.push_back(4);
	dequeSrc.push_back(5);

	FCArray7 dest0;
	dest0.assign(daSrc);

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
		inplace_dynarr<int, 2> di{1, -2};
		auto it = begin(di);
		it = di.unordered_erase(it);
		EXPECT_EQ(-2, *it);
		it = di.unordered_erase(it);
		EXPECT_EQ(end(di), it);

		di = {1, -2};
		unordered_erase(di, 1);
		unordered_erase(di, 0);
		EXPECT_TRUE(di.empty());
	}
}
