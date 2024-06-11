#include "test_classes.h"
#include "views.h"

#include <cstdint>
#include <deque>


namespace view = oel::view;

template<typename ArrayString, typename ArrayChar, typename ArrayBool>
void testConstruct()
{
	ArrayString a;
	decltype(a) b(a);
	ASSERT_EQ(0U, b.size());

	EXPECT_TRUE(typename ArrayString::const_iterator{} == typename ArrayString::iterator{});
	{
		ArrayString c(0, std::string{});
		EXPECT_TRUE(c.empty());
	}
	{
		std::string str = "AbCd";
		ArrayChar test2(oel::from_range, str);
		EXPECT_TRUE( 0 == str.compare(0, 4, test2.data(), test2.size()) );
	}
	ArrayBool db(50, true);
	for (const auto & e : db)
		EXPECT_EQ(true, e);
}

template<typename ArrayMoveOnly, typename ArrayArrayInt>
void testPushBack()
{
	MoveOnly::clearCount();
	{
		ArrayMoveOnly up;

		double const VALUES[] = {-1.1, 2.0};

		up.push_back(MoveOnly{VALUES[0]});
		ASSERT_EQ(1U, up.size());

	#if OEL_HAS_EXCEPTIONS
		MoveOnly::countToThrowOn = 0;
		EXPECT_THROW( up.emplace_back(), TestException );
		ASSERT_EQ(1U, up.size());
	#endif
		up.push_back(MoveOnly{VALUES[1]});
		ASSERT_EQ(2U, up.size());

	#if OEL_HAS_EXCEPTIONS
		MoveOnly::countToThrowOn = 0;
		EXPECT_THROW( up.emplace_back(), TestException );
		ASSERT_EQ(2U, up.size());
	#endif
		up.push_back( std::move(up.back()) );
		ASSERT_EQ(3U, up.size());

		EXPECT_EQ(VALUES[0], *up[0]);
		EXPECT_FALSE(up[1].hasValue());
		EXPECT_EQ(VALUES[1], *up[2]);
	}
	EXPECT_EQ(MoveOnly::nConstructions, MoveOnly::nDestruct);
}

template<typename ArrayTrivialReloc>
void testPushBackTrivialReloc()
{
	TrivialRelocat::clearCount();
	{
		ArrayTrivialReloc da;

		double const VALUES[] = {-1.1, 2.0, -0.7, 9.6};
		std::deque<double> expected;

		da.push_back(TrivialRelocat{VALUES[0]});
		expected.push_back(VALUES[0]);
		ASSERT_EQ(1U, da.size());
		EXPECT_EQ(TrivialRelocat::nConstructions - ssize(da), TrivialRelocat::nDestruct);

		da.emplace_back(VALUES[1]);
		expected.emplace_back(VALUES[1]);
		ASSERT_EQ(2U, da.size());
		EXPECT_EQ(TrivialRelocat::nConstructions - ssize(da), TrivialRelocat::nDestruct);

	#if OEL_HAS_EXCEPTIONS
		TrivialRelocat::countToThrowOn = 1;
		try
		{
			for(;;)
			{
				da.push_back(TrivialRelocat{VALUES[2]});
				expected.push_back(VALUES[2]);
			}
		}
		catch (TestException &) {
		}
		ASSERT_EQ(expected.size(), da.size());
		EXPECT_EQ(TrivialRelocat::nConstructions - ssize(da), TrivialRelocat::nDestruct);
	#endif
		da.emplace_back(VALUES[3]);
		expected.emplace_back(VALUES[3]);
		ASSERT_EQ(expected.size(), da.size());

	#if OEL_HAS_EXCEPTIONS
		TrivialRelocat::countToThrowOn = 0;
		EXPECT_THROW( da.push_back(TrivialRelocat{0}), TestException );
		ASSERT_EQ(expected.size(), da.size());
	#endif
		EXPECT_EQ(TrivialRelocat::nConstructions - ssize(da), TrivialRelocat::nDestruct);

	#if OEL_HAS_EXCEPTIONS
		TrivialRelocat::countToThrowOn = 3;
		try
		{
			for(;;)
			{
				da.push_back(da.front());
				expected.push_back(expected.front());
			}
		}
		catch (TestException &) {
		}
		ASSERT_EQ(expected.size(), da.size());
	#endif
		EXPECT_TRUE(
			std::equal
			(	begin(da), end(da), begin(expected),
				[](const auto & a, double b) { return *a == b; }
			) );
	}
	EXPECT_EQ(TrivialRelocat::nConstructions, TrivialRelocat::nDestruct);
}

template<typename ArrayMoveOnly, typename ArrayTrivialReloc>
void testAssign()
{
	MoveOnly::clearCount();
	{
		double const VALUES[] = {-1.1, 0.4};
		MoveOnly src[] { MoveOnly{VALUES[0]},
						 MoveOnly{VALUES[1]} };
		ArrayMoveOnly test;

		test.assign(oel::view::move(src));

		EXPECT_EQ(2U, test.size());
		EXPECT_EQ(VALUES[0], *test[0]);
		EXPECT_EQ(VALUES[1], *test[1]);

		test.assign(src | view::move);
		EXPECT_EQ(0U, test.size());
	}
	EXPECT_EQ(MoveOnly::nConstructions, MoveOnly::nDestruct);

	TrivialRelocat::clearCount();
	{
		ArrayTrivialReloc dest;
		#if OEL_HAS_EXCEPTIONS
		{
			TrivialRelocat obj{-5.0};
			TrivialRelocat::countToThrowOn = 0;
			EXPECT_THROW(
				dest.assign(view::counted(&obj, 1)),
				TestException );
			EXPECT_TRUE(dest.begin() == dest.end());
		}
		#endif
		EXPECT_EQ(TrivialRelocat::nConstructions, TrivialRelocat::nDestruct);

		dest = {TrivialRelocat{-1.0}};
		EXPECT_EQ(1U, dest.size());
		dest = {TrivialRelocat{1.0}, TrivialRelocat{2.0}};
		EXPECT_EQ(1.0, *dest[0]);
		EXPECT_EQ(2.0, *dest[1]);
		EXPECT_EQ(TrivialRelocat::nConstructions - ssize(dest), TrivialRelocat::nDestruct);
		#if OEL_HAS_EXCEPTIONS
		{
			TrivialRelocat obj{-3.3};
			TrivialRelocat::countToThrowOn = 0;
			EXPECT_THROW(
				dest.assign(view::subrange(&obj, &obj + 1)),
				TestException );
			EXPECT_TRUE(dest.empty() or *dest[1] == 2.0);
		}
		#endif
		{
			dest.clear();
			EXPECT_TRUE(dest.empty());

		#if OEL_HAS_EXCEPTIONS
			TrivialRelocat obj{-1.3};
			TrivialRelocat::countToThrowOn = 0;
			EXPECT_THROW(
				dest.assign(view::counted(&obj, 1)),
				TestException );
			EXPECT_TRUE(dest.empty());
		#endif
		}
	}
	EXPECT_EQ(TrivialRelocat::nConstructions, TrivialRelocat::nDestruct);
}

template<typename ArrayString>
void testAssignStringStream()
{
	ArrayString das;

	std::string * p = nullptr;
	das.assign(view::subrange(p, p));

	EXPECT_EQ(0U, das.size());

	std::stringstream ss{"My computer emits Hawking radiation"};
	std::istream_iterator<std::string> b{ss}, e;
	das.assign(view::subrange(b, e));

	EXPECT_EQ(5U, das.size());

	EXPECT_EQ("My", das[0]);
	EXPECT_EQ("computer", das[1]);
	EXPECT_EQ("emits", das[2]);
	EXPECT_EQ("Hawking", das[3]);
	EXPECT_EQ("radiation", das[4]);

	ArrayString copyDest;

	copyDest.assign(view::counted(das.cbegin(), 2));
	copyDest.assign( view::counted(begin(das), das.size()) );

	EXPECT_TRUE(das == copyDest);

	copyDest.assign(view::subrange(das.cbegin(), das.cbegin() + 1));

	EXPECT_EQ(1U, copyDest.size());
	EXPECT_EQ(das[0], copyDest[0]);

	copyDest.assign(view::counted(das.cbegin() + 2, 3));

	EXPECT_EQ(3U, copyDest.size());
	EXPECT_EQ(das[2], copyDest[0]);
	EXPECT_EQ(das[3], copyDest[1]);
	EXPECT_EQ(das[4], copyDest[2]);

	copyDest = {std::string()};
	EXPECT_EQ("", copyDest[0]);
	copyDest = {das[0], das[4]};
	EXPECT_EQ(2U, copyDest.size());
	EXPECT_EQ(das[4], copyDest[1]);

	copyDest = std::initializer_list<std::string>{};
	EXPECT_TRUE(copyDest.empty());
}

template<typename ArrayDouble, typename ArrayInt>
void testAppend()
{
	{
		ArrayDouble dest;
		// Test append empty std iterator range to empty dynarray
		std::deque<double> src;
		dest.append(src);

		dest.append({});
		EXPECT_EQ(0U, dest.size());

		double const TEST_VAL = 6.6;
		dest.append(2, TEST_VAL);
		dest.append( view::subrange(dest.begin(), dest.end()) );
		EXPECT_EQ(4U, dest.size());
		for (const auto & d : dest)
			EXPECT_EQ(TEST_VAL, d);
	}

	const double arrayA[] = {-1.6, -2.6, -3.6, -4.6};

	ArrayDouble double_dynarr, double_dynarr2;
	double_dynarr.append( view::counted(oel::begin(arrayA), oel::ssize(arrayA)) );
	double_dynarr.append(double_dynarr2);

	{
		ArrayInt int_dynarr;
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
}

template<typename ArrayInt>
void testAppendFromStringStream()
{
	std::stringstream ss("1 2 3 4 5");

	ArrayInt dest;

	std::istream_iterator<int> it(ss);

	// Should hit static_assert
	//dest.insert_range(dest.begin(), view::subrange(it, std::istream_iterator<int>()));

	it = dest.append(view::counted(it, 2));

	dest.append(view::counted(it, 2));

	for (int i = 0; i < ssize(dest); ++i)
		EXPECT_EQ(i + 1, dest[i]);
}

template<typename ArrayDouble, typename ArrayInt>
void testInsertR()
{
	{
		ArrayDouble dest;
		// Test insert empty std iterator range to empty dynarray
		std::deque<double> src;
		dest.insert_range(dest.begin(), src);

		dest.template insert_range< std::initializer_list<double> >(dest.begin(), {});
	}

	const double arrayA[] = {-1.6, -2.6, -3.6, -4.6};

	ArrayDouble double_dynarr, double_dynarr2;
	double_dynarr.insert_range(double_dynarr.begin(), arrayA);
	double_dynarr.insert_range(double_dynarr.end(), double_dynarr2);

	{
		ArrayInt int_dynarr;
		int_dynarr.insert_range(int_dynarr.begin(), std::initializer_list<int>{1, 2, 3, 4});

		double_dynarr.insert_range(double_dynarr.end(), int_dynarr);
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
}

template<typename ArrayTrivialReloc>
void testInsert()
{
	TrivialRelocat::clearCount();
	{
		ArrayTrivialReloc up;

		double const VALUES[] = {-1.1, 0.4, 1.3, 2.2};

		auto & ptr = *up.emplace(begin(up), VALUES[2]);
		EXPECT_EQ(VALUES[2], *ptr);
		ASSERT_EQ(1U, up.size());

	#if OEL_HAS_EXCEPTIONS
		TrivialRelocat::countToThrowOn = 0;
		EXPECT_THROW( up.insert(begin(up), TrivialRelocat{0.0}), TestException );
		ASSERT_EQ(1U, up.size());
	#endif
		up.insert(begin(up), TrivialRelocat{VALUES[0]});
		ASSERT_EQ(2U, up.size());

	#if OEL_HAS_EXCEPTIONS
		TrivialRelocat::countToThrowOn = 0;
		EXPECT_THROW( up.insert(begin(up) + 1, TrivialRelocat{0.0}), TestException );
		ASSERT_EQ(2U, up.size());
	#endif
		up.insert(end(up), TrivialRelocat{VALUES[3]});
		auto & p2 = *up.insert(begin(up) + 1, TrivialRelocat{VALUES[1]});
		EXPECT_EQ(VALUES[1], *p2);
		ASSERT_EQ(4U, up.size());

		auto v = std::begin(VALUES);
		for (const auto & p : up)
		{
			EXPECT_EQ(*v, *p);
			++v;
		}

		auto it = up.insert( begin(up) + 2, std::move(up[2]) );
		EXPECT_FALSE(up[3].hasValue());

		auto const val = *up.back();
		up.insert( end(up) - 1, std::move(up.back()) );
		ASSERT_EQ(6U, up.size());
		EXPECT_FALSE(up.back().hasValue());
		EXPECT_EQ(val, *end(up)[-2]);
	}
	EXPECT_EQ(TrivialRelocat::nConstructions, TrivialRelocat::nDestruct);
}

template<typename ArrayT>
void internalTestErase()
{
	ArrayT d;

	for (int i = 1; i <= 5; ++i)
		d.emplace_back(i);

	auto const s = d.size();
	auto ret = d.erase(begin(d) + 1);
	ret = d.erase(ret);
	EXPECT_EQ(begin(d) + 1, ret);
	ASSERT_EQ(s - 2, d.size());
	EXPECT_EQ(5, static_cast<double>(d.back()));

	ret = d.erase(end(d) - 1);
	EXPECT_EQ(end(d), ret);
	ASSERT_EQ(s - 3, d.size());
	EXPECT_EQ(1, static_cast<double>(d.front()));
}

template<typename ArrayInt, typename ArrayMoveOnly>
void testEraseSingle()
{
	internalTestErase<ArrayInt>();

	MoveOnly::clearCount();
	internalTestErase<ArrayMoveOnly>();
	EXPECT_EQ(MoveOnly::nConstructions, MoveOnly::nDestruct);
}

template<typename ArrayUnsigned>
void testEraseRange()
{
	ArrayUnsigned d;

	for (int i = 1; i <= 5; ++i)
		d.push_back(i);

	auto const s = d.size();
	auto ret = d.erase(begin(d) + 2, begin(d) + 2);
	ASSERT_EQ(s, d.size());
	ret = d.erase(ret - 1, ret + 1);
	EXPECT_EQ(begin(d) + 1, ret);
	ASSERT_EQ(s - 2, d.size());
	EXPECT_EQ(s, d.back());
}

template<typename ArrayInt>
void testEraseToEnd()
{
	ArrayInt li{1, 1, 2, 2, 2, 1, 3};
	li.erase_to_end(std::remove(begin(li), end(li), 1));
	EXPECT_EQ(4U, li.size());
}
