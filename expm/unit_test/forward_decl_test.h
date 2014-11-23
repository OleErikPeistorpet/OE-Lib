#include "dynarray.h"
#include "iterator_range.h"
#include "gtest/gtest.h"
#include <deque>


struct Deleter
{
	static int callCount;

	void operator()(double * p)
	{
		++callCount;
		delete p;
	}
};

typedef std::unique_ptr<double, Deleter> DoublePtr;


class ForwDeclared;

class Outer
{
public:
	oetl::dynarray<ForwDeclared> test;
};
