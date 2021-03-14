#include "fwd.h"

struct Outer
{
	class Inner {};
	friend oel::false_type specify_trivial_relocate(Inner &&);

	void Foo(oel::dynarray<Inner> &);
};
oel::is_trivially_relocatable<Outer::Inner> specify_trivial_relocate(Outer &&);


#include "dynarray.h"

static_assert( !oel::is_trivially_relocatable<Outer>::value, "?" );

void Outer::Foo(oel::dynarray<Inner> & d) { d.max_size(); }


class ForwDeclared;

class ContainSelf
{
	oel::dynarray<ContainSelf> test;

	oel::dynarray<ForwDeclared> test2;
};

class ForwDeclared {};

namespace {
ContainSelf instance{};
}
