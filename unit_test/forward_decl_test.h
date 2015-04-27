#include "dynarray.h"
#include "gtest/gtest.h"


struct ThrowOnConstructT {} const ThrowOnConstruct;
struct ThrowOnMoveOrCopyT {} const ThrowOnMoveOrCopy;

class TestException : public std::exception {};

class MoveOnly
{
	std::unique_ptr<double> val;

public:
	static int nConstruct;
	static int nAssign;
	static int nDestruct;

	static void ClearCount()
	{
		nConstruct = nAssign = nDestruct = 0;
	}

	MoveOnly(double * val) : val(val)
	{	++nConstruct;
	}
	MoveOnly(ThrowOnConstructT)
	{	throw TestException{};
	}
	MoveOnly(ThrowOnMoveOrCopyT)
	{	++nConstruct;
	}
	MoveOnly(MoveOnly && other) noexcept
	{
		val = std::move(other.val);
		++nConstruct;
	}
	MoveOnly & operator =(MoveOnly && other) noexcept
	{
		val = std::move(other.val);
		++nAssign;
		return *this;
	}
	~MoveOnly() { ++nDestruct; }

	operator double *() const { return val.get(); }
};

struct NoAssign
{
	static int nConstruct;
	static int nCopyConstr;
	static int nDestruct;

	static void ClearCount()
	{
		nConstruct = nCopyConstr = nDestruct = 0;
	}

	NoAssign(int val) : val(val)
	{	++nConstruct;
	}
	NoAssign(ThrowOnConstructT)
	{	throw TestException{};
	}
	NoAssign(ThrowOnMoveOrCopyT) : throwOnMove(true)
	{	++nConstruct;
	}
	NoAssign(const NoAssign & other)
	 :	val(other.val)
	{
		if (throwOnMove)
			throw TestException{};

		++nConstruct;
		++nCopyConstr;
	}
	NoAssign(NoAssign && other)
	 :	val(other.val)
	{
		if (throwOnMove)
			throw TestException{};

		other.val = 0;
		++nConstruct;
	}
	void operator =(const NoAssign &) = delete;
	~NoAssign() { ++nDestruct; }

	bool throwOnMove = false;
	std::ptrdiff_t val = 0;
};

namespace oel {
template<> struct is_trivially_relocatable<MoveOnly> : std::true_type {};
template<> struct is_trivially_relocatable<NoAssign> : std::true_type {};
}


class ForwDeclared;

class Outer
{
public:
	oel::dynarray<ForwDeclared> test;
};
