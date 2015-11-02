#include "forward_decl_test.h"
#include "iterator_range.h"
#include "util.h"
#include "compat/std_classes_extra.h"
#include <deque>

/// @cond INTERNAL

class ForwDeclared { char c; };

int MyCounter::nConstruct;
int MyCounter::nDestruct;

using oel::dynarray;
using oel::cbegin;
using oel::cend;
using oel::default_init;

namespace statictest
{
	using Iter = dynarray<float>::iterator;
	using ConstIter = dynarray<float>::const_iterator;

	static_assert(oel::is_trivially_copyable<Iter>::value, "?");
	static_assert(std::is_convertible<Iter, ConstIter>::value, "?");
	static_assert(!std::is_convertible<ConstIter, Iter>::value, "?");

	static_assert(oel::can_memmove_with<Iter, ConstIter>::value, "?");
	static_assert(oel::can_memmove_with<Iter, const float *>::value, "?");
	static_assert(oel::can_memmove_with<float *, ConstIter>::value, "?");
	static_assert(!oel::can_memmove_with<int *, float *>::value, "?");

	static_assert(oel::is_trivially_relocatable< std::tuple<long, dynarray<bool>, double> >::value, "?");
	static_assert(oel::is_trivially_relocatable< std::tuple<> >::value, "?");
	static_assert(!oel::is_trivially_relocatable< std::tuple<int, NontrivialReloc, int> >::value, "?");

	static_assert(oel::is_trivially_copyable< std::reference_wrapper<std::deque<double>> >::value,
				  "Not critical, this assert can be removed");
}

template<typename T>
struct throwingAlloc : public oel::allocator<T>
{
	T * allocate(size_t nObjs)
	{
		if (nObjs > 999)
			throw std::bad_alloc{};

		return oel::allocator<T>::allocate(nObjs);
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

using MyAllocStr = oel::allocator<std::string>;
static_assert(oel::is_trivially_copyable<MyAllocStr>::value, "?");

#if _MSC_VER || __GNUC__ >= 5

TEST_F(dynarrayTest, stdDequeWithOelAlloc)
{
	std::deque<std::string, MyAllocStr> v{"Test"};
	v.emplace_front();
	EXPECT_EQ("Test", v.at(1));
	EXPECT_TRUE(v.front().empty());
}

TEST_F(dynarrayTest, oelDynarrWithStdAlloc)
{
	MoveOnly::ClearCount();
	{
		dynarray< MoveOnly, std::allocator<MoveOnly> > v;

		v.emplace_back(-1.0);
		EXPECT_THROW( v.emplace_back(ThrowOnConstruct), TestException );
		EXPECT_EQ(1, MoveOnly::nConstruct);
		EXPECT_EQ(0, MoveOnly::nDestruct);

		MoveOnly arr[2] {MoveOnly{1.0}, MoveOnly{2.0}};
		v.assign(oel::move_range(arr));
		EXPECT_THROW( v.emplace_back(ThrowOnConstruct), TestException );
		EXPECT_EQ(2, ssize(v));
		EXPECT_TRUE(1.0 == *v[0]);
		EXPECT_TRUE(2.0 == *v[1]);
	}
	EXPECT_EQ(MoveOnly::nConstruct, MoveOnly::nDestruct);
}
#endif

TEST_F(dynarrayTest, construct)
{
	// TODO: Test exception safety of constructors

	{
		oel::allocator<int> a;
		ASSERT_TRUE(oel::allocator<std::string>{} == a);
	}

	{
		Outer o;
	}

	dynarray<std::string> a;
	decltype(a) b(a);
	ASSERT_EQ(0U, b.size());

	dynarray<int> ints(0, {});
	EXPECT_TRUE(ints.empty());

	using Internal = std::deque<double *>;
	dynarray<Internal> test{Internal(5), Internal()};
	EXPECT_EQ(2U, test.size());

	dynarray<Internal> test2(oel::from_range, test);
	EXPECT_EQ(2U, test2.size());
	EXPECT_EQ(5U, test2[0].size());

	dynarray<bool> db(50, true);
	for (const auto & b : db)
		EXPECT_EQ(true, b);
}

TEST_F(dynarrayTest, pushBack)
{
	MoveOnly::ClearCount();
	{
		dynarray<MoveOnly> up;

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

	dynarray< dynarray<int> > nested;
	nested.emplace_back(3, default_init);
	EXPECT_EQ(3U, nested.back().size());
	nested.emplace_back(std::initializer_list<int>{1, 2});
	EXPECT_EQ(2U, nested.back().size());
}

TEST_F(dynarrayTest, pushBackNonTrivialReloc)
{
	using oel::as_signed;
	NontrivialReloc::ClearCount();
	{
		dynarray<NontrivialReloc> mo;

		double const VALUES[] = {-1.1, 2.0, -0.7, 9.6};
		std::deque<double> expected;

		mo.push_back(NontrivialReloc{VALUES[0]});
		expected.push_back(VALUES[0]);
		ASSERT_EQ(1U, mo.size());
		EXPECT_EQ(NontrivialReloc::nConstruct - as_signed(mo.size()), NontrivialReloc::nDestruct);

		mo.emplace_back(VALUES[1], ThrowOnMoveOrCopy);
		expected.emplace_back(VALUES[1]);
		ASSERT_EQ(2U, mo.size());
		EXPECT_EQ(NontrivialReloc::nConstruct - as_signed(mo.size()), NontrivialReloc::nDestruct);

		try
		{
			mo.push_back(NontrivialReloc{VALUES[2]});
			expected.push_back(VALUES[2]);
		}
		catch (TestException &) {
		}
		ASSERT_EQ(expected.size(), mo.size());
		EXPECT_EQ(NontrivialReloc::nConstruct - as_signed(mo.size()), NontrivialReloc::nDestruct);

		try
		{
			mo.push_back(NontrivialReloc{VALUES[2]});
			expected.push_back(VALUES[2]);
		}
		catch (TestException &) {
		}
		ASSERT_EQ(3U, mo.size());
		EXPECT_EQ(NontrivialReloc::nConstruct - as_signed(mo.size()), NontrivialReloc::nDestruct);

		mo.emplace_back(VALUES[3], ThrowOnMoveOrCopy);
		expected.emplace_back(VALUES[3]);
		ASSERT_EQ(4U, mo.size());
		EXPECT_EQ(NontrivialReloc::nConstruct - as_signed(mo.size()), NontrivialReloc::nDestruct);

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

TEST_F(dynarrayTest, assign)
{
	MoveOnly::ClearCount();
	{
		double const VALUES[] = {-1.1, 0.4};
		MoveOnly src[] { MoveOnly{VALUES[0]},
						 MoveOnly{VALUES[1]} };
		dynarray<MoveOnly> test;

		test.assign(oel::move_range(src));

		EXPECT_EQ(2U, test.size());
		EXPECT_EQ(VALUES[0], *test[0]);
		EXPECT_EQ(VALUES[1], *test[1]);

		test.assign(oel::move_range_n(src, 0));
		EXPECT_EQ(0U, test.size());
	}
	EXPECT_EQ(MoveOnly::nConstruct, MoveOnly::nDestruct);

	NontrivialReloc::ClearCount();
	{
		dynarray<NontrivialReloc> dest;
		{
			NontrivialReloc obj{-5.0, ThrowOnMoveOrCopy};
			try
			{	dest.assign(oel::make_range_n(&obj, 1));
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
			{	dest.assign(oel::make_range(&obj, &obj + 1));
			}
			catch (TestException &) {
			}
			EXPECT_TRUE(dest.empty() || dest.at(1) == 2.0);
		}
		{
			dest.clear();
			EXPECT_LE(2U, dest.capacity());
			EXPECT_TRUE(dest.empty());

			NontrivialReloc obj{-1.3, ThrowOnMoveOrCopy};
			try
			{	dest.assign(oel::make_range_n(&obj, 1));
			}
			catch (TestException &) {
			}
			EXPECT_TRUE(dest.empty());
		}
	}
	EXPECT_EQ(NontrivialReloc::nConstruct, NontrivialReloc::nDestruct);
}

TEST_F(dynarrayTest, assignStringStream)
{
	{
		dynarray<std::string> das;

		std::string * p = nullptr;
		das.assign(oel::make_range(p, p));

		EXPECT_EQ(0U, das.size());

		std::stringstream ss{"My computer emits Hawking radiation"};
		std::istream_iterator<std::string> begin{ss};
		std::istream_iterator<std::string> end;
		das.assign(oel::make_range(begin, end));

		EXPECT_EQ(5U, das.size());

		EXPECT_EQ("My", das.at(0));
		EXPECT_EQ("computer", das.at(1));
		EXPECT_EQ("emits", das.at(2));
		EXPECT_EQ("Hawking", das.at(3));
		EXPECT_EQ("radiation", das.at(4));

		decltype(das) copyDest;

		copyDest.assign(oel::make_range_n(cbegin(das), 2));
		copyDest.assign( oel::make_range_n(cbegin(das), das.size()) );

		EXPECT_TRUE(das == copyDest);

		copyDest.assign(oel::make_range(cbegin(das), cbegin(das) + 1));

		EXPECT_EQ(1U, copyDest.size());
		EXPECT_EQ(das[0], copyDest[0]);

		copyDest.assign(oel::make_range_n(cbegin(das) + 2, 3));

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
		dest.append( oel::make_range_n(dest.begin(), dest.size()) );
		EXPECT_EQ(4U, dest.size());
		for (const auto & d : dest)
			EXPECT_EQ(TEST_VAL, d);
	}

	const double arrayA[] = {-1.6, -2.6, -3.6, -4.6};

	dynarray<double> double_dynarr, double_dynarr2;
	double_dynarr.append_rs( oel::make_range_n(oel::begin(arrayA), oel::count(arrayA)) );
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

		it = dest.append_rs(oel::make_range_n(it, 2));

		dest.append_rs(oel::make_range_n(it, 2));

		for (int i = 0; i < ssize(dest); ++i)
			EXPECT_EQ(i + 1, dest[i]);
	}
}

TEST_F(dynarrayTest, insertR)
{
	{
		oel::dynarray<double> dest;
		// Test insert empty std iterator range to empty dynarray
		std::deque<double> src;
		dest.insert_m(dest.begin(), src);

		dest.insert_m< std::initializer_list<double> >(dest.begin(), {});
	}

	const double arrayA[] = {-1.6, -2.6, -3.6, -4.6};

	dynarray<double> double_dynarr, double_dynarr2;
	double_dynarr.insert_rs( double_dynarr.begin(), oel::make_range_n(oel::begin(arrayA), oel::count(arrayA)) );
	double_dynarr.insert_m(double_dynarr.end(), double_dynarr2);

	{
		dynarray<int> int_dynarr;
		int_dynarr.insert_m< std::initializer_list<int> >(int_dynarr.begin(), {1, 2, 3, 4});

		double_dynarr.insert_m(double_dynarr.end(), int_dynarr);
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

TEST_F(dynarrayTest, insert)
{
	MoveOnly::ClearCount();
	{
		dynarray<MoveOnly> up;

		double const VALUES[] = {-1.1, 0.4, 1.3, 2.2};

		auto & p = *up.insert(begin(up), MoveOnly{VALUES[2]});
		EXPECT_EQ(VALUES[2], *p);
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


	dynarray< dynarray<int> > nested;
	nested.resize(3);
	EXPECT_EQ(3, nested.size());
	EXPECT_TRUE(nested.back().empty());

	nested.front().resize(S1);

	nested.resize(1);
	auto cap = nested.capacity();
	EXPECT_EQ(1, nested.size());
	for (auto i : nested.front())
		EXPECT_EQ(0, i);

	auto it = nested.begin();

	nested.resize(cap);
	// Check that no reallocation happened
	EXPECT_EQ(cap, nested.capacity());
	EXPECT_TRUE(nested.begin() == it);

	EXPECT_EQ(S1, nested.front().size());
	EXPECT_TRUE(nested.back().empty());
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

#ifndef OEL_NO_BOOST
TEST_F(dynarrayTest, overAligned)
{
	unsigned int const testAlignment = 32;

	dynarray< oel::aligned_storage_t<testAlignment, testAlignment> > special(0);
	EXPECT_TRUE(special.cbegin() == special.cend());

	special.append(5, {});
	EXPECT_EQ(5, special.size());
	for (const auto & v : special)
		EXPECT_EQ(0, reinterpret_cast<std::uintptr_t>(&v) % testAlignment);

	special.resize(1, oel::default_init);
	special.shrink_to_fit();
	EXPECT_GT(5U, special.capacity());
	EXPECT_EQ(0, reinterpret_cast<std::uintptr_t>(&special.front()) % testAlignment);
}
#endif

TEST_F(dynarrayTest, misc)
{
	{
		dynarray<int> arr[]{ dynarray<int>(2, 1), {1, 1}, {1, 3} };
		dynarray< std::reference_wrapper<const dynarray<int>> > refs{arr[0], arr[1]};
		refs.push_back(arr[2]);
		EXPECT_EQ(3, refs.at(2).get().at(1));
		EXPECT_TRUE(refs.at(0) == refs.at(1));
		EXPECT_TRUE(refs.at(1) != refs.at(2));
	}

	size_t fASrc[] = { 2, 3 };

	dynarray<size_t> daSrc(oel::reserve, 2);
	daSrc.push_back(0);
	daSrc.push_back(2);
	daSrc.insert(begin(daSrc) + 1, 1);
	ASSERT_EQ(3U, daSrc.size());

	ASSERT_NO_THROW(daSrc.at(2));
	ASSERT_THROW(daSrc.at(3), std::out_of_range);

	std::deque<size_t> dequeSrc;
	dequeSrc.push_back(4);
	dequeSrc.push_back(5);

	dynarray<size_t> dest0;
	dest0.reserve(1);
	dest0 = daSrc;

	dest0.append_rs( oel::make_range_n(cbegin(daSrc), daSrc.size()) );
	dest0.append(oel::make_range_n(fASrc, 2));
	auto srcEnd = dest0.append_rs( oel::make_range_n(dequeSrc.begin(), dequeSrc.size()) );
	EXPECT_TRUE(end(dequeSrc) == srcEnd);

	dynarray<size_t> dest1;
	dynarray<size_t>::const_iterator{ dest1.append(daSrc) };
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

/// @endcond
