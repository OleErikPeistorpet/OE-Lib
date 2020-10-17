// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "throw_from_assert.h"
#include "align_allocator.h"

#include "gtest/gtest.h"
#include <memory>
#include <unordered_map>


class TestException : public std::exception {};

struct MyCounter
{
	static int nConstructions;
	static int nDestruct;
	static int countToThrowOn;

	static void ClearCount()
	{
		nConstructions = nDestruct = 0;
		countToThrowOn = -1;
	}

	void ConditionalThrow()
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
	{	ConditionalThrow();
		++nConstructions;
	}
	explicit MoveOnly(double v)
	 :	pVal(&val),
		val(v)
	{
		ConditionalThrow();
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
	{	ConditionalThrow();
		++nConstructions;
	}
	explicit TrivialRelocat(double v) noexcept
	 :	val(new double{v})
	{	++nConstructions;
	}

	TrivialRelocat(const TrivialRelocat & other)
	{
		ConditionalThrow();
		val.reset(new double{*other.val});
		++nConstructions;
	}

	TrivialRelocat & operator =(const TrivialRelocat & other)
	{
		ConditionalThrow();
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
static_assert(std::is_trivially_default_constructible<TrivialDefaultConstruct>::value, "?");
static_assert( !std::is_trivially_copyable<TrivialDefaultConstruct>::value, "?" );

struct NontrivialConstruct : MyCounter
{
	NontrivialConstruct(const NontrivialConstruct &) = delete;

	NontrivialConstruct()
	{
		ConditionalThrow();
		++nConstructions;
	}

	~NontrivialConstruct() { ++nDestruct; }
};
static_assert( !std::is_trivially_default_constructible<NontrivialConstruct>::value, "?" );


struct AllocCounter
{
	static int nAllocations;
	static int nDeallocations;
	static int nConstructCalls;

	static std::unordered_map<void *, std::size_t> sizeFromPtr;

	static void ClearAll()
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

	T * allocate(size_type nElems)
	{
		auto const p = _base::allocate(nElems);
		++AllocCounter::nAllocations;
		AllocCounter::sizeFromPtr[p] = nElems;
		return p;
	}

	void deallocate(T * ptr, size_type nObjects)
	{
		++AllocCounter::nDeallocations;
		// verify that nObjects matches earlier call to allocate
		auto it = AllocCounter::sizeFromPtr.find(ptr);
		ASSERT_TRUE(it != AllocCounter::sizeFromPtr.end());
		EXPECT_EQ(it->second, nObjects);
		AllocCounter::sizeFromPtr.erase(it);

		_base::deallocate(ptr, nObjects);
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
struct StatefulAllocator : std::conditional< UseConstruct, TrackingAllocator<T>, TrackingAllocatorBase<T> >::type
{
	using propagate_on_container_move_assignment = oel::bool_constant<PropagateOnMoveAssign>;

	int id;

	StatefulAllocator(int id_ = 0) : id(id_) {}

	template<typename U>
	friend bool operator==(StatefulAllocator a, StatefulAllocator<U, PropagateOnMoveAssign, UseConstruct> b)
	{ return a.id == b.id; }

	template<typename U>
	friend bool operator!=(StatefulAllocator a, StatefulAllocator<U, PropagateOnMoveAssign, UseConstruct> b)
	{ return !(a == b); }

	using allocator_type = StatefulAllocator;
};


template<typename T>
using dynarrayTrackingAlloc = oel::dynarray< T, TrackingAllocator<T> >;
