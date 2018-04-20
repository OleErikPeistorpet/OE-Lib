#include "dynarray.h"
#include "compat/std_classes_extra.h"
#include "test_classes.h"

#include "gtest/gtest.h"
#include <list>
#include <set>
#include <array>


namespace
{
	static_assert(oel::is_trivially_relocatable< std::array<std::unique_ptr<double>, 4> >::value, "?");

	static_assert(oel::is_trivially_copyable< std::pair<long *, std::array<int, 6>> >::value, "?");
	static_assert(oel::is_trivially_copyable< std::tuple<> >::value, "?");
	static_assert( !oel::is_trivially_copyable< std::tuple<int, NontrivialReloc, int> >::value, "?" );

	static_assert(alignof(oel::aligned_storage_t<32, 16>) == 16, "?");
	static_assert(alignof(oel::aligned_storage_t<64, 64>) == 64, "?");

	static_assert(!oel::can_memmove_with< int *, float * >::value, "?");
	static_assert(!oel::can_memmove_with< int *, std::list<int>::iterator >::value, "?");
	static_assert(!oel::can_memmove_with< int *, std::set<int>::iterator >::value, "?");

	static_assert(oel::is_trivially_copyable< std::reference_wrapper<std::string> >::value,
				  "Not critical, this assert can be removed");
}

template<typename SizeT>
struct DummyRange
{
	using difference_type = typename std::make_signed<SizeT>::type;

	SizeT n;

	SizeT size() const { return n; }
};

TEST(utilTest, indexValid)
{
	using namespace oel;

	DummyRange<unsigned> r1{1};

	EXPECT_TRUE(index_valid(r1, (std::ptrdiff_t) 0));
	EXPECT_FALSE(index_valid(r1, (size_t) 1));
	EXPECT_FALSE(index_valid(r1, -1));
	EXPECT_FALSE(index_valid(r1, (size_t) -1));
	{
		auto const size = std::numeric_limits<unsigned>::max();
		DummyRange<unsigned> r2{size};

		EXPECT_FALSE(index_valid(r2, -2));
		EXPECT_FALSE(index_valid(r2, (long long) -2));
		EXPECT_FALSE(index_valid(r2, (unsigned) -1));
		EXPECT_TRUE(index_valid(r2, size - 1));
		EXPECT_TRUE(index_valid(r2, 0));
	}
	{
		auto const size = as_unsigned(std::numeric_limits<long long>::max());
		DummyRange<unsigned long long> r2{size};

		EXPECT_FALSE(index_valid(r2, (unsigned long long) -2));
		EXPECT_FALSE(index_valid(r2, (long long) -2));
		EXPECT_TRUE(index_valid(r2, size - 1));
	}
}

struct OneSizeT
{
	size_t val;
};

TEST(utilTest, makeUnique)
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

struct RangeWithLargerDiffT
{
	using difference_type = long;

	unsigned short size() const  { return 2; }
};

TEST(utilTest, ssize)
{
	RangeWithLargerDiffT r;
	auto const test = oel::ssize(r);

	static_assert(std::is_same<decltype(test), long const>::value, "?");

	ASSERT_EQ(2, test);
}

TEST(utilTest, derefArgs)
{
	using namespace oel;
	dynarray< std::unique_ptr<double> > d;
	d.push_back(make_unique<double>(3.0));
	d.push_back(make_unique<double>(3.0));
	d.push_back(make_unique<double>(1.0));
	d.push_back(make_unique<double>(2.0));
	d.push_back(make_unique<double>(2.0));
	{
	#if __cplusplus < 201402L and _MSC_VER < 1900
		using Less = std::less<double>;
	#else
		using Less = std::less<>;
	#endif
		std::set< double *, deref_args<Less> > s;
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
	auto last = std::unique(d.begin(), d.end(), deref_args<std::equal_to<double>>());
	EXPECT_EQ(3, last - d.begin());
}

template<typename T>
struct PointerLike
{
	using difference_type = ptrdiff_t;
	using element_type = T;

	T * p;

	T * operator->() const { return p; }
	T & operator *() const { return *p; }
};

TEST(utilTest, toPointerContiguous)
{
	using namespace oel;
	{
		std::basic_string<wchar_t> s;
		using P  = decltype( to_pointer_contiguous(s.begin()) );
		using CP = decltype( to_pointer_contiguous(s.cbegin()) );
		static_assert(std::is_same<P, wchar_t *>::value, "?");
		static_assert(std::is_same<CP, const wchar_t *>::value, "?");

	#if _HAS_CXX17
		std::string_view v;
		using Q = decltype( to_pointer_contiguous(v.begin()) );
		static_assert(std::is_same<Q, const char *>::value, "?");
	#endif
	}
	std::array<int, 3> a;

	using P  = decltype( to_pointer_contiguous(a.begin()) );
	using CP = decltype( to_pointer_contiguous(a.cbegin()) );
	static_assert(std::is_same<P, int *>::value, "?");
	static_assert(std::is_same<CP, const int *>::value, "?");

	auto addr = &a[0];
	using Iter = dynarray_iterator<PointerLike<int>, dynarray<int>>;
	Iter it{{addr}, nullptr, 0};
	static_assert(std::is_same<PointerLike<int>, Iter::pointer>::value, "?");
	auto result = to_pointer_contiguous(it);
	EXPECT_EQ(addr, result);
}
