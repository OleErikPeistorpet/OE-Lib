#include "fixcap_array.h"
#include "range_view.h"

#include "gtest/gtest.h"
#include <deque>

struct Deleter
{
	static int callCount;

	void operator()(double * p)
	{
		++callCount;
		delete p;
	}
};
int Deleter::callCount;

typedef std::unique_ptr<double, Deleter> DoublePtr;


namespace
{

using oel::fixcap_array;

}

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

TEST_F(fixcap_arrayTest, push_back)
{
	Deleter::callCount = 0;
	{
		fixcap_array<DoublePtr, 3> up;

		double const VALUES[] = {-1.1, 2.0};

		up.push_back( DoublePtr(new double(VALUES[0])) );
		ASSERT_EQ(1U, up.size());

		up.push_back( DoublePtr(new double(VALUES[1])) );

		up.push_back( move(up.back()) );
		ASSERT_EQ(3U, up.size());

		EXPECT_EQ(VALUES[0], *up[0]);
		EXPECT_EQ(nullptr, up[1]);
		EXPECT_EQ(VALUES[1], *up[2]);
	}
	EXPECT_EQ(2, Deleter::callCount);
}

TEST_F(fixcap_arrayTest, construct)
{
	//fixcap_array<int, 0> compileWarnOrFail;

	fixcap_array<std::string, 1> a;
	decltype(a) b(a);

	fixcap_array< fixcap_array<int, 8>, 5 > nested;
}

TEST_F(fixcap_arrayTest, assign)
{
	{
		fixcap_array<std::string, 5> das;

		std::string * p = nullptr;
		das.assign(oel::make_iterator_range(p, p));

		EXPECT_EQ(0U, das.size());

		std::stringstream ss{"My computer emits Hawking radiation"};
		std::istream_iterator<std::string> begin{ss};
		std::istream_iterator<std::string> end;
		das.assign(oel::make_iterator_range(begin, end));

		EXPECT_EQ(5U, das.size());

		EXPECT_EQ("My", das.at(0));
		EXPECT_EQ("computer", das.at(1));
		EXPECT_EQ("emits", das.at(2));
		EXPECT_EQ("Hawking", das.at(3));
		EXPECT_EQ("radiation", das.at(4));

		decltype(das) copyDest;

		copyDest.assign(cbegin(das), das.size());

		EXPECT_TRUE(das == copyDest);

		copyDest.assign(oel::make_iterator_range(cbegin(das), cbegin(das) + 1));

		EXPECT_EQ(1U, copyDest.size());
		EXPECT_EQ(das[0], copyDest[0]);

		copyDest.assign(cbegin(das) + 2, 3);

		EXPECT_EQ(3U, copyDest.size());
		EXPECT_EQ(das[2], copyDest[0]);
		EXPECT_EQ(das[3], copyDest[1]);
		EXPECT_EQ(das[4], copyDest[2]);
	}

	Deleter::callCount = 0;
	{
		double const VALUES[] = {-1.1, 0.4};
		DoublePtr src[] { DoublePtr{new double{VALUES[0]}},
						  DoublePtr{new double{VALUES[1]}} };
		fixcap_array<DoublePtr, 2> test;

		test.assign(oel::view::move(src));

		EXPECT_EQ(2U, test.size());
		EXPECT_EQ(VALUES[0], *test[0]);
		EXPECT_EQ(VALUES[1], *test[1]);

		test.assign(std::make_move_iterator(src), 0);
		EXPECT_EQ(0U, test.size());
	}
	EXPECT_EQ(2, Deleter::callCount);
}

TEST_F(fixcap_arrayTest, append)
{
	{
		fixcap_array<double, 4> dest;
		// Test append empty std iterator range to empty fixcap_array
		std::deque<double> src;
		dest.append(src);

		dest.append({});

		double const TEST_VAL = 6.6;
		dest.append({TEST_VAL, TEST_VAL});
		dest.append(dest.begin(), dest.size());
		EXPECT_EQ(4U, dest.size());
		for (const auto & d : dest)
			EXPECT_EQ(TEST_VAL, d);
	}

	const double arrayA[] = {-1.6, -2.6, -3.6, -4.6};
	const int arrayB[] = {1, 2, 3, 4};

	fixcap_array<double, 8> double_dynarr;
	double_dynarr.append(oel::begin(arrayA), oel::ssize(arrayA));

	{
		fixcap_array<int, 4> int_dynarr;
		int_dynarr.append(arrayB);

		double_dynarr.append(int_dynarr);
	}

	ASSERT_EQ(8U, double_dynarr.size());

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

		fixcap_array<int, 4> dest;

		std::istream_iterator<int> it(ss);

		it = dest.append(it, 2);

		dest.append(it, 2);

		for (decltype(dest.size()) i = 0; i < dest.size(); ++i)
			EXPECT_EQ(i + 1, dest[i]);
	}
}

// Test insert.
TEST_F(fixcap_arrayTest, insert)
{
	Deleter::callCount = 0;
	{
		fixcap_array<DoublePtr, 6> up;

		double const VALUES[] = {-1.1, 0.4, 1.3, 2.2};

		auto & p = *up.insert(begin(up), DoublePtr(new double(VALUES[2])));
		EXPECT_EQ(VALUES[2], *p);
		ASSERT_EQ(1U, up.size());
		up.insert( begin(up), DoublePtr(new double(VALUES[0])) );
		up.insert( end(up), DoublePtr(new double(VALUES[3])) );
		ASSERT_EQ(3U, up.size());
		up.insert( begin(up) + 1, DoublePtr(new double(VALUES[1])) );
		ASSERT_EQ(4U, up.size());

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
		ASSERT_EQ(6U, up.size());
		EXPECT_EQ(nullptr, up.back());
		EXPECT_EQ(val, *end(up)[-2]);
	}
	EXPECT_EQ(4, Deleter::callCount);
}

// Test resize.
TEST_F(fixcap_arrayTest, resize)
{
	size_t const S1 = 4;

	fixcap_array<int, S1> d;

	d.resize(S1);
	ASSERT_EQ(S1, d.size());

	int nExcept = 0;
	try
	{
		d.resize(S1 + 1);
	}
	catch (std::length_error &)
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

TEST_F(fixcap_arrayTest, erase)
{
	fixcap_array<int, 5> d;

	for (int i = 0; i < 5; ++i)
		d.push_back(i);

	auto const s = d.size();
	auto r = d.erase(begin(d) + 1, begin(d) + 3);
	ASSERT_EQ(s - 2, d.size());
	r = d.erase(end(d) - 1);
	ASSERT_EQ(s - 3, d.size());
}
