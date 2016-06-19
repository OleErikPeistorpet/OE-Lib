#include "dynarray.h"

/// @cond INTERNAL

class ForwDeclared;

class Outer
{
public:
	oel::dynarray<ForwDeclared> test;
};

/// @endcond
