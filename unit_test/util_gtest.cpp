#include "util.h"
#include "dynarray.h"
#include "gtest/gtest.h"
#include <list>
#include <forward_list>
#include <deque>
#include <array>
#include <set>

/// @cond INTERNAL

// The fixture for testing algorithms and utilities
class utilTest : public ::testing::Test
{
protected:
	utilTest()
	{
		// You can do set-up work for each test here.
	}

	// Objects declared here can be used by all tests.
};

TEST_F(utilTest, eraseUnordered)
{
	using oel::erase_unordered;

	std::deque<std::string> d{"aa", "bb", "cc"};

	erase_unordered(d, 1);
	EXPECT_EQ(2U, d.size());
	EXPECT_EQ("cc", d.back());
	erase_unordered(d, 1);
	EXPECT_EQ(1U, d.size());
	EXPECT_EQ("aa", d.front());
}

TEST_F(utilTest, eraseIf)
{
	using namespace oel;

	std::list<int> li{1, 2, 3, 4, 5, 6};
	std::list<int> const expect{1, 3, 5};
	dynarray<int> test1;
	test1.append(li);

	auto isEven = [](int i) { return i % 2 == 0; };
	erase_if(li, isEven);
	EXPECT_TRUE(expect == li);
	erase_if(test1, isEven);
	EXPECT_EQ(li.size(), test1.size());
	EXPECT_TRUE(std::equal(begin(li), end(li), begin(test1)));
}

TEST_F(utilTest, eraseSuccessiveDup)
{
	using namespace oel;

	std::list<int> li{1, 1, 2, 2, 2, 1, 3};
	dynarray<int> const expect{1, 2, 1, 3};
	dynarray<int> uniqueTest;
	uniqueTest.assign(li);

	erase_successive_dup(li);
	EXPECT_EQ(4U, li.size());
	erase_successive_dup(uniqueTest);
	EXPECT_FALSE(uniqueTest != expect);
}

TEST_F(utilTest, indexValid)
{
	using namespace oel;

	std::list<std::string> li{"aa", "bb"};

	EXPECT_TRUE( index_valid(li, std::int64_t(1)) );
	EXPECT_FALSE(index_valid(li, 2));
	EXPECT_FALSE( index_valid(li, std::uint64_t(-1)) );

	//long l = 1L;
	//EXPECT_TRUE(index_valid(li, l));
}

TEST_F(utilTest, countedView)
{
	using namespace oel;

	static_assert(std::is_nothrow_move_constructible<counted_view<int *>>::value, "?");

	dynarray<int> i{1, 2};
	counted_view<dynarray<int>::const_iterator> test = i;
	EXPECT_EQ(i.size(), test.size());
	EXPECT_TRUE(test.data() == i.data());
	EXPECT_EQ(1, test[0]);
	EXPECT_EQ(2, test[1]);
	test.drop_front();
	EXPECT_EQ(1U, test.size());
	EXPECT_EQ(2, test.end()[-1]);
}

TEST_F(utilTest, viewTransform)
{
	using namespace oel;

	int src[] { 1, 2, 3 };

	struct Fun
	{	int operator()(int i) const
		{
			return i * i;
		}
	};
	dynarray<int> test( view::transform(src, Fun{}) );
	EXPECT_EQ(3U, test.size());
	EXPECT_EQ(1, test[0]);
	EXPECT_EQ(4, test[1]);
	EXPECT_EQ(9, test[2]);

	auto v = make_view_n(src, 2);
	test.append( view::transform(v, [](int & i) { return i++; }) );
	EXPECT_EQ(5U, test.size());
	EXPECT_EQ(1, test[3]);
	EXPECT_EQ(2, test[4]);
	EXPECT_EQ(2, src[0]);
	EXPECT_EQ(3, src[1]);
}

TEST_F(utilTest, copy)
{
	oel::dynarray<int> test = { 0, 1, 2, 3, 4 };
	int test2[5];
	test2[4] = -7;
	auto fitInto = oel::make_view_n(std::begin(test2), 4);

	EXPECT_THROW(oel::copy(test, fitInto), std::out_of_range);

	oel::copy_fit(test, fitInto);
	EXPECT_TRUE(std::equal(begin(test), begin(test) + 4, test2));
	EXPECT_EQ(-7, test2[4]);

	EXPECT_EQ(4, test[4]);
	oel::copy(test2, test);
	EXPECT_EQ(-7, test[4]);
	{
		std::forward_list<std::string> li{"aa", "bb"};
		std::array<std::string, 2> strDest;
		oel::copy_unsafe(oel::view::move_iter_rng(li), begin(strDest));
		EXPECT_EQ("aa", strDest[0]);
		EXPECT_EQ("bb", strDest[1]);
		EXPECT_TRUE(li.begin()->empty());
		EXPECT_TRUE(std::next(li.begin())->empty());
	}
	std::list<std::string> li{"aa", "bb"};
	std::array<std::string, 4> strDest;
	oel::copy_fit(li, strDest);
	EXPECT_EQ("aa", strDest[0]);
	EXPECT_EQ("bb", strDest[1]);
}

TEST_F(utilTest, makeUnique)
{
	auto p1 = oel::make_unique<std::string[]>(5);
	for (int i = 0; i < 5; ++i)
		EXPECT_TRUE(p1[i].empty());

	auto p2 = oel::make_unique<std::list<int>>(4, 6);
	EXPECT_EQ(4U, p2->size());
	EXPECT_EQ(6, p2->front());
	EXPECT_EQ(6, p2->back());
}

struct BlahBy
{
	using difference_type = long;

	unsigned short size() const  { return 2; }
};

TEST_F(utilTest, ssize)
{
	BlahBy bb;
	auto const test = oel::ssize(bb);

	static_assert(std::is_same<decltype(test), long const>::value, "?");

	auto v = std::is_same<short, decltype( oel::as_signed(bb.size()) )>::value;
	EXPECT_TRUE(v);

	ASSERT_EQ(2, test);
}

TEST_F(utilTest, derefArgs)
{
	using namespace oel;
	dynarray< std::unique_ptr<double> > d;
	d.push_back(make_unique<double>(3.0));
	d.push_back(make_unique<double>(3.0));
	d.push_back(make_unique<double>(1.0));
	d.push_back(make_unique<double>(2.0));
	d.push_back(make_unique<double>(2.0));
	{
		std::set< double *, deref_args<std::less<double>> > s;
		for (const auto & p : d)
			s.insert(p.get());

		const auto & constAlias = s;
		double toFind = 2.0;
		EXPECT_TRUE(constAlias.find(&toFind) != constAlias.end());

		EXPECT_EQ(3U, s.size());
		double cmp = 0;
		for (double * v : s)
			EXPECT_DOUBLE_EQ(++cmp, *v);
	}
	auto last = std::unique(d.begin(), d.end(), deref_args<std::equal_to<double>>{});
	EXPECT_EQ(3, last - d.begin());
}

/// @endcond
