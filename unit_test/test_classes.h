#include "core_util.h"

#include <memory>
#include "gtest/gtest.h"

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

/// @endcond
