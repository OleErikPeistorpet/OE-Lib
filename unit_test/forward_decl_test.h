#include "dynarray.h"
#include "gtest/gtest.h"


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
	explicit MoveOnly(double val)
	 :	val(new double{val})
	{	++nConstruct;
	}
	explicit MoveOnly(ThrowOnConstructT)
	{	throw TestException{};
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

class NontrivialReloc : public MyCounter
{
	double val;
	bool throwOnMove = false;

public:
	explicit NontrivialReloc(double val) : val(val)
	{	++nConstruct;
	}
	explicit NontrivialReloc(ThrowOnConstructT)
	{	throw TestException{};
	}
	NontrivialReloc(double val, ThrowOnMoveOrCopyT)
	 :	val(val), throwOnMove(true)
	{	++nConstruct;
	}
	NontrivialReloc(NontrivialReloc && other)
	{
		if (other.throwOnMove)
		{
			other.throwOnMove = false;
			throw TestException{};
		}
		val = other.val;
		++nConstruct;
	}
	NontrivialReloc & operator =(NontrivialReloc && other)
	{
		if (throwOnMove || other.throwOnMove)
		{
			other.throwOnMove = throwOnMove = false;
			throw TestException{};
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

namespace oel {
template<> struct is_trivially_relocatable<MoveOnly> : std::true_type {};
template<> struct is_trivially_relocatable<NontrivialReloc> : std::false_type {};
}


class ForwDeclared;

class Outer
{
public:
	oel::dynarray<ForwDeclared> test;
};
