#include "forward_decl_test.h"
#include "dynarray.h"
#include "iterator_range.h"
#include "gtest/gtest.h"
#include <deque>

class ForwDeclared { char c; };

namespace
{

using oetl::dynarray;

// The fixture for testing dynarray.
class dynarrayTest : public ::testing::Test
{
protected:
	dynarrayTest()
	{
		// You can do set-up work for each test here.
	}

	// Objects declared here can be used by all tests.

	struct Deleter
	{
		static int callCount;

		void operator()(double * p)
		{
			++callCount;
			delete p;
		}
	};

	typedef std::unique_ptr<double, Deleter> DoublePtr;
};

int dynarrayTest::Deleter::callCount{};

TEST_F(dynarrayTest, push_back)
{
	Deleter::callCount = 0;
	{
		dynarray<DoublePtr> up;

		double const VALUES[] = {-1.1, 2.0};

		up.push_back( DoublePtr(new double(VALUES[0])) );
		ASSERT_EQ(1, up.size());

		up.push_back( DoublePtr(new double(VALUES[1])) );

		up.push_back( move(up.back()) );
		ASSERT_EQ(3, up.size());

		EXPECT_EQ(VALUES[0], *up[0]);
		EXPECT_EQ(nullptr, up[1]);
		EXPECT_EQ(VALUES[1], *up[2]);
	}
	EXPECT_EQ(2, Deleter::callCount);
}

TEST_F(dynarrayTest, construct)
{
	{
		Outer o;
	}

	oetl::dynarray<std::string> a;
	decltype(a) b(a);

	dynarray< dynarray<int> > nested;
}

TEST_F(dynarrayTest, assign)
{
	{
		dynarray<std::string> das;

		std::string * p = nullptr;
		das.assign(oetl::make_range(p, p));

		EXPECT_EQ(0, das.size());

		std::stringstream ss{"My computer emits Hawking radiation"};
		std::istream_iterator<std::string> begin{ss};
		std::istream_iterator<std::string> end;
		das.assign(oetl::make_range(begin, end));

		EXPECT_EQ(5, das.size());

		EXPECT_EQ("My", das.at(0));
		EXPECT_EQ("computer", das.at(1));
		EXPECT_EQ("emits", das.at(2));
		EXPECT_EQ("Hawking", das.at(3));
		EXPECT_EQ("radiation", das.at(4));

		decltype(das) copyDest;

		copyDest.assign(cbegin(das), das.size());

		EXPECT_TRUE(das == copyDest);

		copyDest.assign(oetl::make_range(cbegin(das), cbegin(das) + 1));

		EXPECT_EQ(1, copyDest.size());
		EXPECT_EQ(das[0], copyDest[0]);

		copyDest.assign(cbegin(das) + 2, 3);

		EXPECT_EQ(3, copyDest.size());
		EXPECT_EQ(das[2], copyDest[0]);
		EXPECT_EQ(das[3], copyDest[1]);
		EXPECT_EQ(das[4], copyDest[2]);
	}

	Deleter::callCount = 0;
	{
		double const VALUES[] = {-1.1, 0.4};
		DoublePtr src[] { DoublePtr{new double{VALUES[0]}},
						  DoublePtr{new double{VALUES[1]}} };
		dynarray<DoublePtr> test;

		test.assign(oetl::move_range(src));

		EXPECT_EQ(2, test.size());
		EXPECT_EQ(VALUES[0], *test[0]);
		EXPECT_EQ(VALUES[1], *test[1]);

		test.assign(oetl::make_move_iter(src), 0);
		EXPECT_EQ(0, test.size());
	}
	EXPECT_EQ(2, Deleter::callCount);
}

TEST_F(dynarrayTest, append)
{
	{
		oetl::dynarray<double> dest;
		// Test append empty std iterator range to empty dynarray
		std::deque<double> src;
		dest.append(src);

		double const TEST_VAL = 6.6;
		dest.resize(2, TEST_VAL);
		dest.append(dest.begin(), dest.size());
		EXPECT_EQ(4, dest.size());
		for (const auto & d : dest)
			EXPECT_EQ(TEST_VAL, d);
	}

	const double arrayA[] = {-1.6, -2.6, -3.6, -4.6};
	const int arrayB[] = {1, 2, 3, 4};

	dynarray<double> double_dynarr;
	double_dynarr.append(oetl::begin(arrayA), oetl::count(arrayA));

	{
		dynarray<int> int_dynarr;
		int_dynarr.append(arrayB);

		double_dynarr.append(int_dynarr);
	}

	ASSERT_EQ(8, double_dynarr.size());

	EXPECT_EQ(arrayA[0], double_dynarr[0]);
	EXPECT_EQ(arrayA[1], double_dynarr[1]);
	EXPECT_EQ(arrayA[2], double_dynarr[2]);
	EXPECT_EQ(arrayA[3], double_dynarr[3]);

	EXPECT_DOUBLE_EQ(arrayB[0], double_dynarr[4]);
	EXPECT_DOUBLE_EQ(arrayB[1], double_dynarr[5]);
	EXPECT_DOUBLE_EQ(arrayB[2], double_dynarr[6]);
	EXPECT_DOUBLE_EQ(arrayB[3], double_dynarr[7]);

	{
		std::stringstream ss("1 2 3 4 5");

		dynarray<int> dest;

		std::istream_iterator<int> it(ss);

		it = dest.append(it, 2);

		dest.append(it, 2);

		for (decltype(dest.size()) i = 0; i < dest.size(); ++i)
			EXPECT_EQ(i + 1, dest[i]);
	}
}

// Test insert.
TEST_F(dynarrayTest, insert)
{
	Deleter::callCount = 0;
	{
		dynarray<DoublePtr> up;

		double const VALUES[] = {-1.1, 0.4, 1.3, 2.2};

		auto & p = *up.insert(begin(up), DoublePtr(new double(VALUES[2])));
		EXPECT_EQ(VALUES[2], *p);
		ASSERT_EQ(1, up.size());
		up.insert( begin(up), DoublePtr(new double(VALUES[0])) );
		up.insert( end(up), DoublePtr(new double(VALUES[3])) );
		ASSERT_EQ(3, up.size());
		up.insert( begin(up) + 1, DoublePtr(new double(VALUES[1])) );
		ASSERT_EQ(4, up.size());

		auto v = std::begin(VALUES);
		for (const auto & p : up)
		{
			EXPECT_EQ(*v, *p);
			++v;
		}

		auto it = up.insert( begin(up) + 2, move(up[2]) );
		EXPECT_EQ(up[2], *it);
		EXPECT_EQ(nullptr, up[3]);

		auto const val = *up.back();
		up.insert( end(up) - 1, move(up.back()) );
		ASSERT_EQ(6, up.size());
		EXPECT_EQ(nullptr, up.back());
		EXPECT_EQ(val, *end(up)[-2]);
	}
	EXPECT_EQ(4, Deleter::callCount);

	//d.insert(begin(d), 0);
	//ASSERT_EQ(1, d.size());
	//d.insert(begin(d), 1);
	//ASSERT_EQ(2, d.size());
	//d.insert(end(d), 2);
	//ASSERT_EQ(3, d.size());
	//d.insert(begin(d) + 1, 3);
	//ASSERT_EQ(4, d.size());
	//d.insert(end(d) - 1, 4);
	//ASSERT_EQ(5, d.size());

	//int cmp[] = {1, 3, 0, 4, 2};
	//EXPECT_TRUE(std::equal(cbegin(d), cend(d), cmp));
}

// Test resize.
TEST_F(dynarrayTest, resize)
{
	dynarray<int> d;

	size_t const S1 = 4;
	size_t const VAL = 313;

	d.resize(S1, VAL);
	ASSERT_EQ(S1, d.size());

	unsigned int nExcept = 0;
	try
	{
		d.resize((size_t)-8);
	}
	catch (std::bad_alloc &)
	{
		++nExcept;
	}
	EXPECT_EQ(1, nExcept);

	EXPECT_EQ(S1, d.size());
	for (const auto & e : d)
	{
		EXPECT_EQ(VAL, e);
	}
}

TEST_F(dynarrayTest, erase)
{
	dynarray<int> d;

	for (int i = 0; i < 5; ++i)
		d.push_back(i);

	//auto r = t.erase(t.begin() + 2, t.begin() + 1);

	//dynarray<int> dest;
	//auto it = t.begin();
	//t.append(it, 5);
	//dest.append(it, 5);

	auto const s = d.size();
	auto r = d.erase(begin(d) + 1, begin(d) + 3);
	ASSERT_EQ(s - 2, d.size());
	r = d.erase(end(d) - 1);
	ASSERT_EQ(s - 3, d.size());
}

}  // namespace

int main(int argc, char **argv)
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}