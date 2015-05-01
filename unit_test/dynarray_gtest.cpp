#include "forward_decl_test.h"
#include "iterator_range.h"
#include "util.h"
#include <deque>

class ForwDeclared { char c; };

int MoveOnly::nConstruct;
int MoveOnly::nAssign;
int MoveOnly::nDestruct;

int NoAssign::nConstruct;
int NoAssign::nCopyConstr;
int NoAssign::nDestruct;

using oel::dynarray;
using oel::cbegin;
using oel::cend;
using oel::default_init;

template<typename T>
struct throwingAlloc
{
	using value_type = T;

	T * allocate(size_t nObjs)
	{
		if (nObjs > 999)
			throw std::bad_alloc{};

		return static_cast<T *>(::operator new[](sizeof(T) * nObjs));
	}

	void deallocate(T * ptr, size_t)
	{
		::operator delete[](ptr);
	}
};

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

TEST_F(dynarrayTest, stdDequeWithOelAlloc)
{
	using MyAlloc = oel::allocator<std::string>;
	static_assert(oel::is_trivially_copyable<MyAlloc>::value, "?");

	std::deque<std::string, MyAlloc> v{"Test"};
	v.emplace_front();
	EXPECT_EQ("Test", v.back());
	EXPECT_TRUE(v.front().empty());
}

TEST_F(dynarrayTest, construct)
{
	{
		Outer o;
	}

	using Iter = dynarray<std::string>::iterator;
	using ConstIter = dynarray<std::string>::const_iterator;

	static_assert(oel::is_trivially_copyable<Iter>::value, "?");
	static_assert(std::is_convertible<Iter, ConstIter>::value, "?");
	static_assert(!std::is_convertible<ConstIter, Iter>::value, "?");

	dynarray<std::string> a;
	decltype(a) b(a);
	ASSERT_EQ(0U, b.size());

	dynarray<int> ints(0, {});
	EXPECT_TRUE(b.empty());

	using Internal = std::deque<double *>;
	dynarray<Internal> test{Internal(5), Internal()};
	EXPECT_EQ(2U, test.size());
	EXPECT_EQ(5U, test[0].size());
}

TEST_F(dynarrayTest, push_back)
{
	MoveOnly::ClearCount();
	{
		dynarray<MoveOnly> up;

		double const VALUES[] = {-1.1, 2.0};

		up.push_back( MoveOnly(new double(VALUES[0])) );
		ASSERT_EQ(1U, up.size());

		EXPECT_THROW( up.emplace_back(ThrowOnConstruct), TestException );
		ASSERT_EQ(1U, up.size());

		up.push_back( MoveOnly(new double(VALUES[1])) );
		ASSERT_EQ(2U, up.size());

		EXPECT_THROW( up.emplace_back(ThrowOnConstruct), TestException );
		ASSERT_EQ(2U, up.size());

		up.push_back( std::move(up.back()) );
		ASSERT_EQ(3U, up.size());

		EXPECT_EQ(VALUES[0], *up[0]);
		EXPECT_EQ(nullptr, up[1]);
		EXPECT_EQ(VALUES[1], *up[2]);
	}
	EXPECT_EQ(MoveOnly::nConstruct, MoveOnly::nDestruct);

	dynarray< dynarray<int> > nested;
	nested.emplace_back(3, default_init);
	EXPECT_EQ(3U, nested.back().size());
	nested.emplace_back(std::initializer_list<int>{1, 2});
	EXPECT_EQ(2U, nested.back().size());
}

TEST_F(dynarrayTest, assign)
{
	{
		dynarray<std::string> das;

		std::string * p = nullptr;
		das.assign(oel::make_range(p, p));

		EXPECT_EQ(0U, das.size());

		std::stringstream ss{"My computer emits Hawking radiation"};
		std::istream_iterator<std::string> begin{ss};
		std::istream_iterator<std::string> end;
		//das.assign(begin, 5); // should not compile unless no boost
		das.assign(oel::make_range(begin, end));

		EXPECT_EQ(5U, das.size());

		EXPECT_EQ("My", das.at(0));
		EXPECT_EQ("computer", das.at(1));
		EXPECT_EQ("emits", das.at(2));
		EXPECT_EQ("Hawking", das.at(3));
		EXPECT_EQ("radiation", das.at(4));

		decltype(das) copyDest;

		copyDest.assign(cbegin(das), das.size());

		EXPECT_TRUE(das == copyDest);

		copyDest.assign(oel::make_range(cbegin(das), cbegin(das) + 1));

		EXPECT_EQ(1U, copyDest.size());
		EXPECT_EQ(das[0], copyDest[0]);

		copyDest.assign(cbegin(das) + 2, 3);

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

	MoveOnly::ClearCount();
	{
		double const VALUES[] = {-1.1, 0.4};
		MoveOnly src[] { MoveOnly{new double{VALUES[0]}},
						 MoveOnly{new double{VALUES[1]}} };
		dynarray<MoveOnly> test;

		test.assign(oel::move_range(src));

		EXPECT_EQ(2U, test.size());
		EXPECT_EQ(VALUES[0], *test[0]);
		EXPECT_EQ(VALUES[1], *test[1]);

		test.assign(std::make_move_iterator(src), 0);
		EXPECT_EQ(0U, test.size());
	}
	EXPECT_EQ(MoveOnly::nConstruct, MoveOnly::nDestruct);
}

TEST_F(dynarrayTest, append)
{
	{
		oel::dynarray<double> dest;
		// Test append empty std iterator range to empty dynarray
		std::deque<double> src;
		dest.append(src);

		dest.append({});

		double const TEST_VAL = 6.6;
		dest.append(2, TEST_VAL);
		dest.append(dest.begin(), dest.size());
		EXPECT_EQ(4U, dest.size());
		for (const auto & d : dest)
			EXPECT_EQ(TEST_VAL, d);
	}

	const double arrayA[] = {-1.6, -2.6, -3.6, -4.6};

	dynarray<double> double_dynarr, double_dynarr2;
	double_dynarr.append(oel::begin(arrayA), oel::count(arrayA));
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

	{
		std::stringstream ss("1 2 3 4 5");

		dynarray<int> dest;

		std::istream_iterator<int> it(ss);

		it = dest.append(it, 2);

		dest.append(it, 2);

		for (int i = 0; i < static_cast<int>(dest.size()); ++i)
			EXPECT_EQ(i + 1, dest[i]);
	}
}

// Test insert.
TEST_F(dynarrayTest, insert)
{
	MoveOnly::ClearCount();
	{
		dynarray<MoveOnly> up;

		double const VALUES[] = {-1.1, 0.4, 1.3, 2.2};

		auto & p = *up.insert(begin(up), MoveOnly(new double(VALUES[2])));
		EXPECT_EQ(VALUES[2], *p);
		ASSERT_EQ(1U, up.size());

		EXPECT_THROW( up.emplace(begin(up), ThrowOnConstruct), TestException );
		ASSERT_EQ(1U, up.size());

		up.insert( begin(up), MoveOnly(new double(VALUES[0])) );
		ASSERT_EQ(2U, up.size());

		EXPECT_THROW( up.emplace(begin(up) + 1, ThrowOnConstruct), TestException );
		ASSERT_EQ(2U, up.size());

		up.insert( end(up), MoveOnly(new double(VALUES[3])) );
		auto & p2 = *up.insert( begin(up) + 1, MoveOnly(new double(VALUES[1])) );
		EXPECT_EQ(VALUES[1], *p2);
		ASSERT_EQ(4U, up.size());

		auto v = std::begin(VALUES);
		for (const auto & p : up)
		{
			EXPECT_EQ(*v, *p);
			++v;
		}

		auto it = up.insert( begin(up) + 2, std::move(up[2]) );
		EXPECT_EQ(up[2], *it);
		EXPECT_EQ(nullptr, up[3]);

		auto const val = *up.back();
		up.insert( end(up) - 1, std::move(up.back()) );
		ASSERT_EQ(6U, up.size());
		EXPECT_EQ(nullptr, up.back());
		EXPECT_EQ(val, *end(up)[-2]);
	}
	EXPECT_EQ(MoveOnly::nConstruct, MoveOnly::nDestruct);
}

// Test resize.
TEST_F(dynarrayTest, resize)
{
	dynarray<int, throwingAlloc<int>> d;

	size_t const S1 = 4;

	d.resize(S1);
	ASSERT_EQ(S1, d.size());

	int nExcept = 0;
	try
	{
		d.resize((size_t)-8, default_init);
	}
	catch (std::bad_alloc &)
	{
		++nExcept;
	}
	EXPECT_EQ(1, nExcept);

	EXPECT_EQ(S1, d.size());
	for (const auto & e : d)
	{
		EXPECT_EQ(0, e);
	}
}

TEST_F(dynarrayTest, erase)
{
	dynarray<int> d;

	for (int i = 0; i < 5; ++i)
		d.push_back(i);

	auto const s = d.size();
	auto ret = d.erase(begin(d) + 1, begin(d) + 3);
	ASSERT_EQ(begin(d) + 1, ret);
	ASSERT_EQ(s - 2, d.size());

	ret = d.erase(end(d) - 1);
	ASSERT_EQ(end(d), ret);
	ASSERT_EQ(s - 3, d.size());
}

TEST_F(dynarrayTest, misc)
{
	size_t fASrc[] = { 2, 3 };

	dynarray<size_t> daSrc(oel::reserve, 2);
	daSrc.push_back(0);
	daSrc.push_back(2);
	daSrc.insert(begin(daSrc) + 1, 1);
	ASSERT_EQ(3U, daSrc.size());

	std::deque<size_t> dequeSrc;
	dequeSrc.push_back(4);
	dequeSrc.push_back(5);

	dynarray<size_t> dest0;
	dest0.reserve(1);
	dest0 = daSrc;

	dest0.append(cbegin(daSrc), daSrc.size());
	dest0.append(fASrc, 2);
	auto srcEnd = dest0.append(dequeSrc.begin(), dequeSrc.size());
	EXPECT_TRUE(end(dequeSrc) == srcEnd);

	dynarray<size_t> dest1;
	dynarray<size_t>::const_iterator it = dest1.append(daSrc);
	dest1.append(fASrc);
	dest1.append(dequeSrc);

	{
		dynarray<int> di{1, -2};
		auto it = begin(di);
		it = erase_unordered(di, it);
		EXPECT_EQ(-2, *it);
		it = erase_unordered(di, it);
		EXPECT_EQ(end(di), it);
	}

	auto cap = dest1.capacity();
	dest1.pop_back();
	dest1.pop_back();
	dest1.shrink_to_fit();
	EXPECT_GT(cap, dest1.capacity());
}
