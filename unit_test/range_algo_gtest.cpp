#include "range_algo.h"
#include "dynarray.h"

#include "gtest/gtest.h"
#include <list>
#include <forward_list>
#include <deque>
#include <array>
#include <valarray>

namespace view = oel::view;

TEST(rangeTest, eraseUnstable)
{
	using oel::erase_unstable;

	std::deque<std::string> d{"aa", "bb", "cc"};

	erase_unstable(d, 1);
	EXPECT_EQ(2U, d.size());
	EXPECT_EQ("cc", d.back());
	erase_unstable(d, 1);
	EXPECT_EQ(1U, d.size());
	EXPECT_EQ("aa", d.front());
}

TEST(rangeTest, eraseIf)
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

TEST(rangeTest, eraseSuccessiveDup)
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

TEST(rangeTest, countedView)
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

#if !defined OEL_NO_BOOST
TEST(rangeTest, viewTransform)
{
	int src[] { 1, 2, 3 };

	struct Fun
	{	int operator()(int i) const
		{
			return i * i;
		}
	};
	oel::dynarray<int> test( view::transform(src, Fun{}) );
	EXPECT_EQ(3U, test.size());
	EXPECT_EQ(1, test[0]);
	EXPECT_EQ(4, test[1]);
	EXPECT_EQ(9, test[2]);
	{
	auto f = std::function<int(int &)>( [](int & i) { return i++; } );
	test.append(view::transform_n(src, 2, f));
	EXPECT_EQ(5U, test.size());
	EXPECT_EQ(1, test[3]);
	EXPECT_EQ(2, test[4]);
	EXPECT_EQ(2, src[0]);
	EXPECT_EQ(3, src[1]);
	}
	auto r = oel::make_iterator_range(std::begin(src) + 1, std::end(src));
	auto f = [](int i) { return i; };
	test.assign( view::transform(r, std::ref(f)) );
	EXPECT_EQ(2U, test.size());
	EXPECT_EQ(src[1], test[0]);
	EXPECT_EQ(src[2], test[1]);
}
#endif

TEST(rangeTest, copyUnsafe)
{
	std::valarray<int> src(2);
	src[0] = 1;
	src[1] = 2;
	std::valarray<int> dest(2);
	oel::copy_unsafe(src, begin(dest));
	EXPECT_EQ(1, dest[0]);
	EXPECT_EQ(2, dest[1]);
}

TEST(rangeTest, copy)
{
	oel::dynarray<int> test = { 0, 1, 2, 3, 4 };
	int test2[5];
	test2[4] = -7;
	auto fitInto = view::counted(std::begin(test2), 4);

OEL_WHEN_EXCEPTIONS_ON(
	EXPECT_THROW(oel::copy(test, fitInto), std::out_of_range);
)
	auto success = oel::copy_fit(test, fitInto);
	EXPECT_TRUE(std::equal(begin(test), begin(test) + 4, test2));
	EXPECT_EQ(-7, test2[4]);
	EXPECT_FALSE(success);

	ASSERT_EQ(4, test[4]);
	auto l = oel::copy(test2, test).dest_last;
	EXPECT_EQ(-7, test[4]);
	EXPECT_TRUE(end(test) == l);
	{
		std::forward_list<std::string> li{"aa", "bb"};
		std::array<std::string, 2> strDest;
		auto sLast = oel::copy(view::move(li), strDest).source_last;
		EXPECT_EQ("aa", strDest[0]);
		EXPECT_EQ("bb", strDest[1]);
		EXPECT_TRUE(li.begin()->empty());
		EXPECT_TRUE(std::next(li.begin())->empty());
		EXPECT_TRUE(end(li) == sLast.base());
	}
	std::list<std::string> li{"aa", "bb"};
	std::array<std::string, 4> strDest;
	success = oel::copy_fit(li, strDest);
	EXPECT_EQ("aa", strDest[0]);
	EXPECT_EQ("bb", strDest[1]);
	EXPECT_TRUE(success);
}

template<typename Container>
void testAppend()
{
	Container c;

	oel::append(c, std::initializer_list<int>{1, 2});
	EXPECT_EQ(2U, c.size());
	oel::append(c, 3, -1);
	EXPECT_EQ(5U, c.size());
	EXPECT_EQ( 2, *std::next(c.begin()) );
	EXPECT_EQ(-1, c.back());
}

TEST(rangeTest, append)
{
	testAppend< std::list<int> >();
	testAppend< oel::dynarray<int> >();
}
