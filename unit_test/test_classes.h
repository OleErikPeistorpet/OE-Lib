#pragma once

#include "align_allocator.h"

#include "gtest/gtest.h"
#include <memory>
#include <unordered_map>

//! @cond INTERNAL

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
	std::unique_ptr<double> val;

public:
	MoveOnly()
	{	ConditionalThrow();
		++nConstructions;
	}
	explicit MoveOnly(double v)
	 :	val(new double{v})
	{	ConditionalThrow();
		++nConstructions;
	}

	MoveOnly(MoveOnly && other) noexcept
	 :	val(std::move(other.val))
	{	++nConstructions;
	}

	MoveOnly & operator =(MoveOnly && other) noexcept
	{
		val = std::move(other.val);
		return *this;
	}

	~MoveOnly() { ++nDestruct; }

	const double * get() const { return val.get(); }

	double operator *() const { return *val; }
};
oel::true_type specify_trivial_relocate(MoveOnly);

class NontrivialReloc : public MyCounter
{
	double val;
	short unused;

public:
	explicit NontrivialReloc(double v)
	 :	val(v)
	{	++nConstructions;
	}

	NontrivialReloc(const NontrivialReloc & other)
	{
		ConditionalThrow();
		val = other.val;
		++nConstructions;
	}

	NontrivialReloc & operator =(const NontrivialReloc & other)
	{
		ConditionalThrow();
		val = other.val;
		return *this;
	}

	~NontrivialReloc() { ++nDestruct; }

	operator double() const { return val; }

	double operator *() const { return val; }

	const double * get() const { return &val; }
};
oel::false_type specify_trivial_relocate(NontrivialReloc);

struct TrivialDefaultConstruct
{
	TrivialDefaultConstruct() = default;
	TrivialDefaultConstruct(const TrivialDefaultConstruct &) {}
};
static_assert(oel::is_trivially_default_constructible<TrivialDefaultConstruct>::value, "?");
static_assert( !oel::is_trivially_copyable<TrivialDefaultConstruct>::value, "?" );

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
static_assert( !oel::is_trivially_default_constructible<NontrivialConstruct>::value, "?" );


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
};


template<typename T>
using dynarrayTrackingAlloc = oel::dynarray< T, TrackingAllocator<T> >;

//! @endcond
