#include "util.h"
#include "dynarray.h"
#include "range_algo.h"
#include "range_view.h"

#include "gtest/gtest.h"
#include <list>
#include <set>
#include <forward_list>
#include <functional>

/// @cond INTERNAL

// The fixture for testing utilities
class utilTest : public ::testing::Test
{
protected:
	utilTest()
	{
		// You can do set-up work for each test here.
	}

	// Objects declared here can be used by all tests.
};

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

#ifndef OEL_NO_BOOST
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
	test.append(view::transform( v, std::function<int(int &)>([](int & i) { return i++; }) ));
	EXPECT_EQ(5U, test.size());
	EXPECT_EQ(1, test[3]);
	EXPECT_EQ(2, test[4]);
	EXPECT_EQ(2, src[0]);
	EXPECT_EQ(3, src[1]);

	auto r = make_iterator_range(begin(src) + 1, end(src));
	auto f = [](int i) { return i; };
	test.assign( view::transform(r, std::ref(f)) );
	EXPECT_EQ(2U, test.size());
	EXPECT_EQ(src[1], test[0]);
	EXPECT_EQ(src[2], test[1]);
}
#endif

struct OneSizeT
{
	size_t val;
};

TEST_F(utilTest, makeUnique)
{
	auto ps = oel::make_unique_default<std::string[]>(2);
	EXPECT_TRUE(ps[0].empty());
	EXPECT_TRUE(ps[1].empty());

	{
		auto p = oel::make_unique<OneSizeT>(7U);
		EXPECT_EQ(7U, p->val);

		auto a = oel::make_unique<OneSizeT[]>(5);
		for (size_t i = 0; i < 5; ++i)
			EXPECT_EQ(0U, a[i].val);
	}
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
