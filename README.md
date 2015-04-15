# OE-Lib
A very fast substitute for C++ std::vector (and std::copy)

The library is distributed under the Boost Software License, and is header only, just include and go.

Unlike the containers in most standard library implementations, over-aligned types (as used by AVX instructions) are supported with no special action by the user. This feature does require that you have boost headers (1.56 or newer).

For better display of dynarray and the iterators in the Visual Studio debugger, copy `oe_lib.natvis` to:
`<My Documents>\Visual Studio 2013\Visualizers` (should also work for newer Visual Studio, just different year in path)

To use dense matrixes, quaternions, etc. from the Eigen library in dynarray: include `compat/eigen_dense.h`

You may want to customize what happens when an assert fails; this can be done by defining OEL_HALT() to a custom action. Or do whatever suitable, see 'debug.h'.
