#include "range_view.h"
#include "compat/std_classes_extra.h"

#include "gtest/gtest.h"
#include <cstdint>
#include <deque>
#include <memory>

/// @cond INTERNAL
struct ThrowOnConstructT {} const ThrowOnConstruct;
struct ThrowOnMoveOrCopyT {} const ThrowOnMoveOrCopy;

class TestException : public std::exception {};

struct MyCounter
{
	static int nConstruct;
	static int nDestruct;

	static void ClearCount()
	{
		nConstruct = nDestruct = 0;
	}
};

class MoveOnly : public MyCounter
{
	std::unique_ptr<double> val;

public:
	explicit MoveOnly(double v)
	 :	val(new double{v})
	{	++nConstruct;
	}
	explicit MoveOnly(ThrowOnConstructT)
	{	OEL_THROW(TestException{});
	}
	MoveOnly(MoveOnly && other) noexcept
	 :	val(std::move(other.val))
	{	++nConstruct;
	}
	MoveOnly & operator =(MoveOnly && other) noexcept
	{
		val = std::move(other.val);
		return *this;
	}
	~MoveOnly() { ++nDestruct; }

	operator double *() const { return val.get(); }
};
oel::is_trivially_relocatable<std::unique_ptr<double>> specify_trivial_relocate(MoveOnly);

class NontrivialReloc : public MyCounter
{
	double val;
	bool throwOnMove = false;

public:
	explicit NontrivialReloc(double v) : val(v)
	{	++nConstruct;
	}
	explicit NontrivialReloc(ThrowOnConstructT)
	{	OEL_THROW(TestException{});
	}
	NontrivialReloc(double v, ThrowOnMoveOrCopyT)
	 :	val(v), throwOnMove(true)
	{	++nConstruct;
	}
	NontrivialReloc(NontrivialReloc && other)
	{
		if (other.throwOnMove)
		{
			other.throwOnMove = false;
			OEL_THROW(TestException{});
		}
		val = other.val;
		++nConstruct;
	}
	NontrivialReloc(const NontrivialReloc & other)
	{
		if (other.throwOnMove)
		{
			OEL_THROW(TestException{});
		}
		val = other.val;
		++nConstruct;
	}
	NontrivialReloc & operator =(const NontrivialReloc & other)
	{
		if (throwOnMove || other.throwOnMove)
		{
			throwOnMove = false;
			OEL_THROW(TestException{});
		}
		val = other.val;
		return *this;
	}
	~NontrivialReloc() { ++nDestruct; }

	operator double() const
	{
		return val;
	}
};
oel::false_type specify_trivial_relocate(NontrivialReloc);

static_assert(oel::is_trivially_copyable<NontrivialReloc>::value == false, "?");


using oel::make_iterator_range;
namespace view = oel::view;

template<typename ArrayString, typename ArrayChar, typename ArrayBool>
void testConstruct()
{
	// TODO: Test exception safety of constructors

	ArrayString a;
	decltype(a) b(a);
	ASSERT_EQ(0U, b.size());

	EXPECT_TRUE(ArrayString::const_iterator{} == ArrayString::iterator{});
	{
		ArrayString c(0, std::string{});
		EXPECT_TRUE(c.empty());
	}
	{
		std::string str = "AbCd";
		ArrayChar test2(str);
		EXPECT_TRUE( 0 == str.compare(0, 4, test2.data(), test2.size()) );
	}
	ArrayBool db(50, true);
	for (const auto & e : db)
		EXPECT_EQ(true, e);
}

template<typename ArrayMoveOnly, typename ArrayArrayInt>
void testPushBack()
{
	MoveOnly::ClearCount();
	{
		ArrayMoveOnly up;

		double const VALUES[] = {-1.1, 2.0};

		up.push_back(MoveOnly{VALUES[0]});
		ASSERT_EQ(1U, up.size());

		EXPECT_THROW( up.emplace_back(ThrowOnConstruct), TestException );
		ASSERT_EQ(1U, up.size());

		up.push_back(MoveOnly{VALUES[1]});
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

	ArrayArrayInt nested;
	nested.emplace_back(3, oel::default_init);
	EXPECT_EQ(3U, nested.back().size());
	nested.emplace_back(std::initializer_list<int>{1, 2});
	EXPECT_EQ(2U, nested.back().size());
}

template<typename ArrayNontrivialReloc>
void testPushBackNonTrivialReloc()
{
	NontrivialReloc::ClearCount();
	{
		ArrayNontrivialReloc mo;

		double const VALUES[] = {-1.1, 2.0, -0.7, 9.6};
		std::deque<double> expected;

		mo.push_back(NontrivialReloc{VALUES[0]});
		expected.push_back(VALUES[0]);
		ASSERT_EQ(1U, mo.size());
		EXPECT_EQ(NontrivialReloc::nConstruct - ssize(mo), NontrivialReloc::nDestruct);

		mo.emplace_back(VALUES[1], ThrowOnMoveOrCopy);
		expected.emplace_back(VALUES[1]);
		ASSERT_EQ(2U, mo.size());
		EXPECT_EQ(NontrivialReloc::nConstruct - ssize(mo), NontrivialReloc::nDestruct);

		try
		{
			mo.push_back(NontrivialReloc{VALUES[2]});
			expected.push_back(VALUES[2]);
		}
		catch (TestException &) {
		}
		ASSERT_EQ(expected.size(), mo.size());
		EXPECT_EQ(NontrivialReloc::nConstruct - ssize(mo), NontrivialReloc::nDestruct);

		try
		{
			mo.push_back(NontrivialReloc{VALUES[2]});
			expected.push_back(VALUES[2]);
		}
		catch (TestException &) {
		}
		ASSERT_EQ(3U, mo.size());
		EXPECT_EQ(NontrivialReloc::nConstruct - ssize(mo), NontrivialReloc::nDestruct);

		mo.emplace_back(VALUES[3], ThrowOnMoveOrCopy);
		expected.emplace_back(VALUES[3]);
		ASSERT_EQ(4U, mo.size());
		EXPECT_EQ(NontrivialReloc::nConstruct - ssize(mo), NontrivialReloc::nDestruct);

		EXPECT_THROW( mo.emplace_back(ThrowOnConstruct), TestException );
		ASSERT_EQ(4U, mo.size());

		try
		{
			mo.push_back( std::move(mo.front()) );
			expected.push_back(expected.front());
		}
		catch (TestException &) {
		}
		ASSERT_EQ(expected.size(), mo.size());

		try
		{
			mo.push_back( std::move(mo.front()) );
			expected.push_back(expected.front());
		}
		catch (TestException &) {
		}
		EXPECT_EQ(5U, mo.size());

		EXPECT_TRUE( std::equal(begin(mo), end(mo), begin(expected)) );
	}
	EXPECT_EQ(NontrivialReloc::nConstruct, NontrivialReloc::nDestruct);
}

template<typename ArrayMoveOnly, typename ArrayNontrivialReloc>
void testAssign()
{
	MoveOnly::ClearCount();
	{
		double const VALUES[] = {-1.1, 0.4};
		MoveOnly src[] { MoveOnly{VALUES[0]},
						 MoveOnly{VALUES[1]} };
		ArrayMoveOnly test;

		test.assign(oel::view::move(src));

		EXPECT_EQ(2U, test.size());
		EXPECT_EQ(VALUES[0], *test[0]);
		EXPECT_EQ(VALUES[1], *test[1]);

		test.assign(oel::view::move_n(src, 0));
		EXPECT_EQ(0U, test.size());
	}
	EXPECT_EQ(MoveOnly::nConstruct, MoveOnly::nDestruct);

	NontrivialReloc::ClearCount();
	{
		ArrayNontrivialReloc dest;
		{
			NontrivialReloc obj{-5.0, ThrowOnMoveOrCopy};
			try
			{	dest.assign(view::counted(&obj, 1));
			}
			catch (TestException &) {
			}
			EXPECT_TRUE(dest.begin() == dest.end());
		}
		EXPECT_EQ(NontrivialReloc::nConstruct, NontrivialReloc::nDestruct);

		dest = {NontrivialReloc{-1.0}};
		EXPECT_EQ(1U, dest.size());
		dest = {NontrivialReloc{1.0}, NontrivialReloc{2.0}};
		EXPECT_DOUBLE_EQ(1.0, dest.at(0));
		EXPECT_DOUBLE_EQ(2.0, dest.at(1));
		EXPECT_EQ(NontrivialReloc::nConstruct - ssize(dest), NontrivialReloc::nDestruct);
		{
			NontrivialReloc obj{-3.3, ThrowOnMoveOrCopy};
			try
			{	dest.assign(make_iterator_range(&obj, &obj + 1));
			}
			catch (TestException &) {
			}
			EXPECT_TRUE(dest.empty() || dest.at(1) == 2.0);
		}
		{
			dest.clear();
			EXPECT_TRUE(dest.empty());

			NontrivialReloc obj{-1.3, ThrowOnMoveOrCopy};
			try
			{	dest.assign(view::counted(&obj, 1));
			}
			catch (TestException &) {
			}
			EXPECT_TRUE(dest.empty());
		}
	}
	EXPECT_EQ(NontrivialReloc::nConstruct, NontrivialReloc::nDestruct);
}

template<typename ArrayString>
void testAssignStringStream()
{
	ArrayString das;

	std::string * p = nullptr;
	das.assign(make_iterator_range(p, p));

	EXPECT_EQ(0U, das.size());

	std::stringstream ss{"My computer emits Hawking radiation"};
	std::istream_iterator<std::string> begin{ss};
	std::istream_iterator<std::string> end;
	das.assign(make_iterator_range(begin, end));

	EXPECT_EQ(5U, das.size());

	EXPECT_EQ("My", das.at(0));
	EXPECT_EQ("computer", das.at(1));
	EXPECT_EQ("emits", das.at(2));
	EXPECT_EQ("Hawking", das.at(3));
	EXPECT_EQ("radiation", das.at(4));

	ArrayString copyDest;

	copyDest.assign(view::counted(cbegin(das), 2));
	copyDest.assign( view::counted(cbegin(das), das.size()) );

	EXPECT_TRUE(das == copyDest);

	copyDest.assign(make_iterator_range(cbegin(das), cbegin(das) + 1));

	EXPECT_EQ(1U, copyDest.size());
	EXPECT_EQ(das[0], copyDest[0]);

	copyDest.assign(view::counted(cbegin(das) + 2, 3));

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

template<typename ArrayDouble, typename ArrayInt>
void testAppend()
{
	{
		ArrayDouble dest;
		// Test append empty std iterator range to empty dynarray
		std::deque<double> src;
		dest.append(src);

		dest.append({});

		double const TEST_VAL = 6.6;
		dest.append(2, TEST_VAL);
		dest.append( make_iterator_range(dest.begin(), dest.end()) );
		EXPECT_EQ(4U, dest.size());
		for (const auto & d : dest)
			EXPECT_EQ(TEST_VAL, d);
	}

	const double arrayA[] = {-1.6, -2.6, -3.6, -4.6};

	ArrayDouble double_dynarr, double_dynarr2;
	double_dynarr.append_ret_src( view::counted(oel::begin(arrayA), oel::ssize(arrayA)) );
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
	//dest.insert_r(dest.begin(), make_iterator_range(it, std::istream_iterator<int>()));

	it = dest.append_ret_src(view::counted(it, 2));

	dest.append_ret_src(view::counted(it, 2));

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
		dest.insert_r(dest.begin(), src);

		dest.insert_r< std::initializer_list<double> >(dest.begin(), {});
	}

	const double arrayA[] = {-1.6, -2.6, -3.6, -4.6};

	ArrayDouble double_dynarr, double_dynarr2;
	double_dynarr.insert_r(double_dynarr.begin(), arrayA);
	double_dynarr.insert_r(double_dynarr.end(), double_dynarr2);

	{
		ArrayInt int_dynarr;
		int_dynarr.insert_r(int_dynarr.begin(), std::initializer_list<int>{1, 2, 3, 4});

		double_dynarr.insert_r(double_dynarr.end(), int_dynarr);
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

template<typename ArrayMoveOnly>
void testInsert()
{
	MoveOnly::ClearCount();
	{
		ArrayMoveOnly up;

		double const VALUES[] = {-1.1, 0.4, 1.3, 2.2};

		auto & ptr = *up.insert(begin(up), MoveOnly{VALUES[2]});
		EXPECT_EQ(VALUES[2], *ptr);
		ASSERT_EQ(1U, up.size());

		EXPECT_THROW( up.emplace(begin(up), ThrowOnConstruct), TestException );
		ASSERT_EQ(1U, up.size());

		up.insert(begin(up), MoveOnly{VALUES[0]});
		ASSERT_EQ(2U, up.size());

		EXPECT_THROW( up.emplace(begin(up) + 1, ThrowOnConstruct), TestException );
		ASSERT_EQ(2U, up.size());

		up.insert(end(up), MoveOnly{VALUES[3]});
		auto & p2 = *up.insert(begin(up) + 1, MoveOnly{VALUES[1]});
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

template<typename ArrayInt, typename Exception>
void testResize()
{
	ArrayInt d;

	size_t const S1 = 4;

	d.resize(S1);
	ASSERT_EQ(S1, d.size());

	int nExcept = 0;
	try
	{
		d.resize((size_t)-8, oel::default_init);
	}
	catch (Exception &)
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
	EXPECT_DOUBLE_EQ(s, d.back());

	ret = d.erase(end(d) - 1);
	EXPECT_EQ(end(d), ret);
	ASSERT_EQ(s - 3, d.size());
	EXPECT_DOUBLE_EQ(1, d.front());
}

template<typename ArrayInt, typename ArrayNontrivialReloc>
void testEraseSingle()
{
	internalTestErase<ArrayInt>();

	NontrivialReloc::ClearCount();
	internalTestErase<ArrayNontrivialReloc>();
	EXPECT_EQ(NontrivialReloc::nConstruct, NontrivialReloc::nDestruct);
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

template<typename ArrayInt, typename ArrayReferenceWrapperArrayConstOfInt>
void testWithRefWrapper()
{
	ArrayInt arr[]{ ArrayInt(/*size*/ 2, 1), {1, 1}, {1, 3} };
	ArrayReferenceWrapperArrayConstOfInt refs{arr[0], arr[1]};
	refs.push_back(arr[2]);
	EXPECT_EQ(3, refs.at(2).get().at(1));
	EXPECT_TRUE(refs.at(0) == refs.at(1));
	EXPECT_TRUE(refs.at(1) != refs.at(2));
}

/// @endcond
