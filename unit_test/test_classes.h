#include "core_util.h"

#include <memory>
#include "gtest/gtest.h"

/// @cond INTERNAL

struct ThrowOnConstructT {} const ThrowOnConstruct;
struct ThrowOnMoveOrCopyT {} const ThrowOnMoveOrCopy;

class TestException : public std::exception {};

struct MyCounter
{
	static int nConstructions;
	static int nDestruct;

	static void ClearCount()
	{
		nConstructions = nDestruct = 0;
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
	explicit MoveOnly(ThrowOnConstructT)
	{	OEL_THROW(TestException{});
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

	operator double *() const { return val.get(); }
};
oel::is_trivially_relocatable<std::unique_ptr<double>> specify_trivial_relocate(MoveOnly);

class NontrivialReloc : public MyCounter
{
	double val;
	bool throwOnMove = false;

public:
	explicit NontrivialReloc(double v) : val(v)
	{	++nConstructions;
	}
	explicit NontrivialReloc(ThrowOnConstructT)
	{	OEL_THROW(TestException{});
	}
	NontrivialReloc(double v, ThrowOnMoveOrCopyT)
	 :	val(v), throwOnMove(true)
	{	++nConstructions;
	}
	NontrivialReloc(NontrivialReloc && other)
	{
		if (other.throwOnMove)
		{
			other.throwOnMove = false;
			OEL_THROW(TestException{});
		}
		val = other.val;
		++nConstructions;
	}
	NontrivialReloc(const NontrivialReloc & other)
	{
		if (other.throwOnMove)
		{
			OEL_THROW(TestException{});
		}
		val = other.val;
		++nConstructions;
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


struct AllocCounter
{
	static int nAllocations;
	static int nDeallocations;
	static int nConstructCalls;
};

template<typename T>
struct TrackingAllocator : oel::allocator<T>, AllocCounter
{
	using _base = oel::allocator<T>;

	using size_type = typename std::allocator_traits<_base>::size_type;

	T * allocate(size_type nObjects)
	{
		++nAllocations;
		return _base::allocate(nObjects);
	}
	void deallocate(T * ptr, size_type nObjects)
	{
		++nDeallocations;
		_base::deallocate(ptr, nObjects);
	}

	template<typename U, typename... Args>
	void construct(U * raw, Args &&... args)
	{
		++nConstructCalls;
		_base::construct(raw, std::forward<Args>(args)...);
	}
};

/// @endcond
