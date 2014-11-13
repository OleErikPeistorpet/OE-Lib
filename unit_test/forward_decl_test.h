#include "dynarray.h"

class ForwDeclared;

class Outer
{
public:
	oetl::dynarray<ForwDeclared> test;
};
