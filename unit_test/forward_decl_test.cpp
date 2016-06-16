/// @cond INTERNAL

#include "user_traits.h"

struct Outer
{
	class Inner {};
	friend oel::false_type specify_trivial_relocate(Inner &&);

	void Foo(oel::dynarray<Inner> &);
};
oel::is_trivially_relocatable<Outer::Inner> specify_trivial_relocate(Outer &&);


#include "dynarray.h"

static_assert( !oel::is_trivially_relocatable<Outer>::value, "?" );


class ForwDeclared;

class ContainSelf
{
	oel::dynarray<ContainSelf> test;

	oel::dynarray<ForwDeclared> test2;
};

class ForwDeclared { char a; };

ContainSelf instance{};

/// @endcond
