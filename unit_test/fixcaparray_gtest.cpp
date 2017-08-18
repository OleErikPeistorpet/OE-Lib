#include "fixcap_array.h"
#include "generic_array_test.h"

using oel::fixcap_array;

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

TEST_F(fixcap_arrayTest, pushBack)
{
	testPushBack< fixcap_array<MoveOnly, 5>, fixcap_array<fixcap_array<int, 3>, 2> >();
}

TEST_F(fixcap_arrayTest, pushBackNonTrivialReloc)
{
	testPushBackNonTrivialReloc< fixcap_array<NontrivialReloc, 5> >();
}

TEST_F(fixcap_arrayTest, assign)
{
	testAssign< fixcap_array<MoveOnly, 5>, fixcap_array<NontrivialReloc, 5> >();
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
	testInsert< fixcap_array<MoveOnly, 6> >();
}

TEST_F(fixcap_arrayTest, resize)
{
	testResize< fixcap_array<int, 4>, std::length_error >();
}

TEST_F(fixcap_arrayTest, eraseSingle)
{
	testEraseSingle< fixcap_array<int, 5>, fixcap_array<NontrivialReloc, 5> >();
}

TEST_F(fixcap_arrayTest, eraseRange)
{
	testEraseRange< fixcap_array<unsigned, 5> >();
}

TEST_F(fixcap_arrayTest, eraseToEnd)
{
	testEraseToEnd< fixcap_array<int, 7> >();
}

#ifndef OEL_NO_BOOST
TEST_F(fixcap_arrayTest, overAligned)
{
	unsigned int const testAlignment = 32;

	fixcap_array< oel::aligned_storage_t<testAlignment, testAlignment>, 5 > special(0);
	EXPECT_TRUE(special.cbegin() == special.cend());

	special.append(5, {});
	EXPECT_EQ(5U, special.size());
	for (const auto & v : special)
		EXPECT_EQ(0U, reinterpret_cast<std::uintptr_t>(&v) % testAlignment);

	special.resize(1, oel::default_init);
	EXPECT_EQ(0U, reinterpret_cast<std::uintptr_t>(&special.front()) % testAlignment);
}
#endif

TEST_F(fixcap_arrayTest, withRefWrapper)
{
	using ArrayInt = fixcap_array<int, 2>;
	testWithRefWrapper< ArrayInt, fixcap_array<std::reference_wrapper<ArrayInt const>, 3> >();
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

	ASSERT_NO_THROW(daSrc.at(2));
	ASSERT_THROW(daSrc.at(3), std::out_of_range);

	std::deque<size_t> dequeSrc;
	dequeSrc.push_back(4);
	dequeSrc.push_back(5);

	FCArray7 dest0;
	dest0.assign(daSrc);

	dest0.append( oel::make_iterator_range(cbegin(daSrc), cend(daSrc) - 1) );
	dest0.pop_back();
	dest0.append(oel::make_view_n(fASrc, 2));
	dest0.pop_back();
	auto srcEnd = dest0.append( oel::make_view_n(dequeSrc.begin(), dequeSrc.size()) );
	EXPECT_TRUE(end(dequeSrc) == srcEnd);

	FCArray7 dest1;
	FCArray7::const_iterator( dest1.append(daSrc) );
	dest1.append(fASrc);
	dest1.append(dequeSrc);
	EXPECT_EQ(FCArray7::max_size(), dest1.size());

	{
		fixcap_array<int, 2> di{1, -2};
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
