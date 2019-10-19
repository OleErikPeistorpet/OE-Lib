#include "generic_array_test.h"
#include "fixcap_array.h"
#include "range_algo.h"

using oel::fixcap_array;

using FcaString       = fixcap_array<std::string, 1>;
using FcaMoveMayThrow = fixcap_array<NontrivialConstruct, 1>;

static_assert(    std::is_nothrow_move_constructible<FcaString>{}, "?");
static_assert(not std::is_nothrow_move_constructible<FcaMoveMayThrow >{}, "?");
static_assert(not std::is_nothrow_move_assignable<FcaMoveMayThrow>{}, "?");

// The fixture for testing fixcap_array.
class fixcap_arrayTest : public ::testing::Test
{
protected:
	fixcap_arrayTest()
	{
		// You can do set-up work for each test here.
	}

	// Objects declared here can be used by all tests.
};

TEST_F(fixcap_arrayTest, construct)
{
	//fixcap_array<int, 0> compileWarnOrFail;

	using Internal = std::deque<double *>;
	fixcap_array<Internal, 2> test{Internal(5), Internal()};
	EXPECT_EQ(2U, test.size());

	testConstruct< fixcap_array<std::string, 1>, fixcap_array<char, 4>, fixcap_array<bool, 50> >();
}

TEST_F(fixcap_arrayTest, pushBackMoveOnly)
{
	testPushBack< fixcap_array<MoveOnly, 5>, fixcap_array<fixcap_array<int, 3>, 2> >();
}

TEST_F(fixcap_arrayTest, pushBackTrivialReloc)
{
	testPushBackTrivialReloc< fixcap_array<TrivialRelocat, 5> >();
}

TEST_F(fixcap_arrayTest, assign)
{
	testAssign< fixcap_array<MoveOnly, 5>, fixcap_array<TrivialRelocat, 5> >();
}

TEST_F(fixcap_arrayTest, assignStringStream)
{
	testAssignStringStream< fixcap_array<std::string, 5> >();
}

TEST_F(fixcap_arrayTest, append)
{
	testAppend<	fixcap_array<double, 8>, fixcap_array<int, 4> >();
}

TEST_F(fixcap_arrayTest, appendFromStringStream)
{
	testAppendFromStringStream<	fixcap_array<int, 5> >();
}

TEST_F(fixcap_arrayTest, insertR)
{
	testInsertR< fixcap_array<double, 8>, fixcap_array<int, 4> >();
}

TEST_F(fixcap_arrayTest, insert)
{
	testInsert< fixcap_array<TrivialRelocat, 6> >();
}

TEST_F(fixcap_arrayTest, resize)
{
	fixcap_array<int, 4> d;

	size_t const S1 = 4;

	d.resize(S1);
	ASSERT_EQ(S1, d.size());

OEL_WHEN_EXCEPTIONS_ON(
	EXPECT_THROW(d.resize_default_init(d.max_size() + 1), std::length_error);
	EXPECT_EQ(S1, d.size());
)
	for (const auto & e : d)
	{
		EXPECT_EQ(0, e);
	}


	fixcap_array< fixcap_array<int, 4>, 4 > nested;
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

TEST_F(fixcap_arrayTest, eraseSingle)
{
	testEraseSingle< fixcap_array<int, 5>, fixcap_array<MoveOnly, 5> >();
}

TEST_F(fixcap_arrayTest, eraseRange)
{
	testEraseRange< fixcap_array<unsigned, 5> >();
}

TEST_F(fixcap_arrayTest, eraseToEnd)
{
	testEraseToEnd< fixcap_array<int, 7> >();
}

TEST_F(fixcap_arrayTest, overAligned)
{
	unsigned int const testAlignment = 32;
	struct Type
	{	oel::aligned_storage_t<testAlignment, testAlignment> a;
	};
	fixcap_array<Type, 5> special(0);
	EXPECT_TRUE(special.cbegin() == special.cend());

	special.insert(special.begin(), Type());
	special.emplace(special.begin());
	special.emplace(special.begin() + 1);
	EXPECT_EQ(3U, special.size());
	for (const auto & v : special)
		EXPECT_EQ(0U, reinterpret_cast<std::uintptr_t>(&v) % testAlignment);

	special.erase_unstable(special.end() - 1);
	special.erase_unstable(special.begin());
	EXPECT_EQ(0U, reinterpret_cast<std::uintptr_t>(&special.front()) % testAlignment);
}

TEST_F(fixcap_arrayTest, misc)
{
	size_t fASrc[] = { 2, 3 };

	using FCArray3 = fixcap_array<size_t, 3>;
	using FCArray7 = fixcap_array<size_t, 7>;

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
		fixcap_array<int, 2> di{1, -2};
		auto it = begin(di);
		it = di.erase_unstable(it);
		EXPECT_EQ(-2, *it);
		it = di.erase_unstable(it);
		EXPECT_EQ(end(di), it);

		di = {1, -2};
		erase_unstable(di, 1);
		erase_unstable(di, 0);
		EXPECT_TRUE(di.empty());
	}
}
