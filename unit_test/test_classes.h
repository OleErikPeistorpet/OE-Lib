#pragma once

#include "core_util.h"

#include <memory>
#include "gtest/gtest.h"

/// @cond INTERNAL

struct ThrowOnConstructT {} const throwOnConstruct;

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
				OEL_THROW(TestException{});
		}
	}
};

class MoveOnly : public MyCounter
{
	std::unique_ptr<double> val;

public:
	explicit MoveOnly(double v)
	 :	val(new double{v})
	{	++nConstructions;
	}

	explicit MoveOnly(ThrowOnConstructT) { OEL_THROW(TestException{}); }

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

	double operator*() const { return *val; }
};
oel::true_type specify_trivial_relocate(MoveOnly);

class NontrivialReloc : public MyCounter
{
	double val;

public:
	explicit NontrivialReloc(double v)
	 :	val(v)
	{	++nConstructions;
	}

	explicit NontrivialReloc(ThrowOnConstructT) { OEL_THROW(TestException{}); }

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

	operator double() const
	{
		return val;
	}
};
oel::false_type specify_trivial_relocate(NontrivialReloc);

struct TrivialDefaultConstruct
{
	TrivialDefaultConstruct() = default;
	TrivialDefaultConstruct(TrivialDefaultConstruct &&) = delete;
	TrivialDefaultConstruct(const TrivialDefaultConstruct &) = delete;
};

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


struct AllocCounter
{
	static int nAllocations;
	static int nDeallocations;
	static int nConstructCalls;
};

template<typename T>
struct TrackingAllocator : oel::allocator<T>
{
	using _base = oel::allocator<T>;

	using size_type = typename std::allocator_traits<_base>::size_type;

	T * allocate(size_type nObjects)
	{
		++AllocCounter::nAllocations;
		return _base::allocate(nObjects);
	}
	void deallocate(T * ptr, size_type nObjects)
	{
		++AllocCounter::nDeallocations;
		_base::deallocate(ptr, nObjects);
	}

	template<typename U, typename... Args>
	void construct(U * raw, Args &&... args)
	{
		++AllocCounter::nConstructCalls;
		_base::construct(raw, std::forward<Args>(args)...);
	}
};

template<typename T, bool PropagateOnMoveAssign>
struct StatefulAllocator : TrackingAllocator<T>
{
	using propagate_on_container_move_assignment = oel::bool_constant<PropagateOnMoveAssign>;

	int id;

	StatefulAllocator(int id_ = 0) : id(id_) {}
};

template<typename T, typename U, bool PropagateOnMoveAssign>
bool operator==(StatefulAllocator<T, PropagateOnMoveAssign> a, StatefulAllocator<U, PropagateOnMoveAssign> b)
{ return a.id == b.id; }

template<typename T, typename U, bool PropagateOnMoveAssign>
bool operator!=(StatefulAllocator<T, PropagateOnMoveAssign> a, StatefulAllocator<U, PropagateOnMoveAssign> b)
{ return !(a == b); }

/// @endcond
