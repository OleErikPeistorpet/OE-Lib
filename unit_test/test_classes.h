// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "allocator.h"

#include "gtest/gtest.h"
#include <memory>
#include <unordered_map>


#ifndef HAS_STD_PMR
	#if __has_include(<memory_resource>)
	#define HAS_STD_PMR  1
	#else
	#define HAS_STD_PMR  0
	#endif
#endif


class MemoryLeakDetector;

extern MemoryLeakDetector* leakDetector;


#if __cpp_lib_ranges >= 201911

#include <ranges>

inline auto ToMutableBeginSizeView(const int(& arr)[1])
{
	return std::views::drop_while(
		arr,
		[](const auto &) { return false; } );
}

#else

#define NO_VIEWS_ISTREAM  1

struct MutableBeginSizeView
{
	const int * _begin;

	const int * begin() { return _begin; }

	size_t size() { return 1; }
};

inline auto ToMutableBeginSizeView(const int(& arr)[1])
{
	return MutableBeginSizeView{arr};
}
#endif

class TestException : public std::exception {};

struct MyCounter
{
	static int nConstructions;
	static int nDestruct;
	static int countToThrowOn;

	static void clearCount()
	{
		nConstructions = nDestruct = 0;
		countToThrowOn = -1;
	}

	void conditionalThrow()
	{
		if (0 <= countToThrowOn)
		{
			if (0 == countToThrowOn--)
				OEL_THROW(TestException{}, "");
		}
	}
};

class MoveOnly : public MyCounter
{
	double * pVal{};
	double val{};

public:
	MoveOnly()
	{	conditionalThrow();
		++nConstructions;
	}
	explicit MoveOnly(double v)
	 :	pVal(&val),
		val(v)
	{
		conditionalThrow();
		++nConstructions;
	}

	MoveOnly(MoveOnly && other) noexcept
	 :	pVal(other.pVal ? &val : nullptr),
		val(other.val)
	{
		other.pVal = nullptr;
		++nConstructions;
	}

	MoveOnly & operator =(MoveOnly && other) noexcept
	{	// The point here is to make pVal null in case of self move assignment,
		// in order to test for that happening unexpectedly
		pVal = nullptr;
		if (other.pVal)
		{
			val = *other.pVal;
			pVal = &val;
			other.pVal = nullptr;
		}
		return *this;
	}

	~MoveOnly() { ++nDestruct; }

	bool hasValue() const { return pVal != nullptr; }

	double operator *() const        { return *pVal; }
	explicit operator double() const { return *pVal; }
};
oel::false_type specify_trivial_relocate(MoveOnly);

class TrivialRelocat : public MyCounter
{
	std::unique_ptr<double> val;

public:
	TrivialRelocat()
	{	conditionalThrow();
		++nConstructions;
	}
	explicit TrivialRelocat(double v) noexcept
	 :	val(new double{v})
	{	++nConstructions;
	}

	TrivialRelocat(const TrivialRelocat & other)
	{
		conditionalThrow();
		val.reset(new double{*other.val});
		++nConstructions;
	}

	TrivialRelocat & operator =(const TrivialRelocat & other)
	{
		conditionalThrow();
		val.reset(new double{*other.val});
		return *this;
	}

	~TrivialRelocat() { ++nDestruct; }

	bool hasValue() const { return val != nullptr; }

	double operator *() const        { return *val; }
	explicit operator double() const { return *val; }
};
oel::true_type specify_trivial_relocate(TrivialRelocat);

struct TrivialDefaultConstruct
{
	TrivialDefaultConstruct() = default;
	TrivialDefaultConstruct(const TrivialDefaultConstruct &) {}
};
static_assert(std::is_trivially_default_constructible<TrivialDefaultConstruct>::value);
static_assert( !std::is_trivially_copyable<TrivialDefaultConstruct>::value );

struct NontrivialConstruct : MyCounter
{
	NontrivialConstruct(const NontrivialConstruct &) = delete;

	NontrivialConstruct()
	{
		conditionalThrow();
		++nConstructions;
	}

	~NontrivialConstruct() { ++nDestruct; }
};
static_assert( !std::is_trivially_default_constructible<NontrivialConstruct>::value );


struct AllocCounter
{
	static int nAllocations;
	static int nDeallocations;
	static int nConstructCalls;

	static std::unordered_map<void *, std::size_t> sizeFromPtr;

	static void clearAll()
	{
		nAllocations = 0;
		nDeallocations = 0;
		nConstructCalls = 0;

		sizeFromPtr.clear();
	}
};

template<typename T>
struct TrackingAllocatorBase : oel::allocator<T>
{
	using _base = oel::allocator<T>;

	using size_type = typename std::allocator_traits<_base>::size_type;

	T * allocate(size_type count)
	{
		auto const p = _base::allocate(count);
		if (p)
			++AllocCounter::nAllocations;

		AllocCounter::sizeFromPtr[p] = count;
		return p;
	}

	T * reallocate(T * ptr, size_type count)
	{
		if (ptr)
			++AllocCounter::nDeallocations;

		++AllocCounter::nAllocations;
		ptr = _base::reallocate(ptr, count);
		AllocCounter::sizeFromPtr[ptr] = count;
		return ptr;
	}

	void deallocate(T * ptr, size_type count)
	{
		if (ptr)
			++AllocCounter::nDeallocations;

		// verify that count matches earlier call to allocate
		auto it = AllocCounter::sizeFromPtr.find(ptr);
		ASSERT_TRUE(it != AllocCounter::sizeFromPtr.end());
		EXPECT_EQ(it->second, count);
		AllocCounter::sizeFromPtr.erase(it);

		_base::deallocate(ptr, count);
	}
};

template<typename T>
struct TrackingAllocator : TrackingAllocatorBase<T>
{
	template<typename U, typename... Args>
	void construct(U * raw, Args &&... args)
	{
		++AllocCounter::nConstructCalls;
		new(raw) T(std::forward<Args>(args)...);;
	}

	using Alloc = void;
};

template<typename T, bool PropagateOnMoveAssign = false, bool UseConstruct = true>
struct StatefulAllocator : std::conditional_t< UseConstruct, TrackingAllocator<T>, TrackingAllocatorBase<T> >
{
	using propagate_on_container_move_assignment = oel::bool_constant<PropagateOnMoveAssign>;

	int id;

	explicit StatefulAllocator(int id_ = 0) : id(id_) {}

	friend bool operator==(StatefulAllocator a, StatefulAllocator b) { return a.id == b.id; }

	friend bool operator!=(StatefulAllocator a, StatefulAllocator b) { return !(a == b); }

	using allocator_type = StatefulAllocator;
};


template<typename T>
using dynarrayTrackingAlloc = oel::dynarray< T, TrackingAllocator<T> >;
