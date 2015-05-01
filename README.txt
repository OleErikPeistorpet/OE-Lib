Obscure Efficient Library
-------------------------
A cross-platform, very fast substitute for C++ std::vector (and std::copy)

The library is distributed under the Boost Software License, and is header only, just include and go.

Unlike the containers in most standard library implementations, over-aligned types (as used by SSE and AVX instructions) are supported with no special action by the user. This feature does require that you have Boost headers (1.56 or newer). If you don't use Boost, at least compilation will fail with such types. Using std::vector, a runtime crash would be expected.

Visual Studio 2013 or GCC 4.7 is required. Clang has not been tested, but should work.

For better display of dynarray and the iterators in the Visual Studio debugger, copy `oe_lib.natvis` to:
`<My Documents>\Visual Studio 2013\Visualizers` (should also work for newer Visual Studio, just different year in path)

To use dense matrixes, quaternions, etc. from the Eigen library in dynarray: include `compat/eigen_dense.h`

If runtime checks are desired when running without a debugger, you should customize what happens when an assert fails. This can be done by defining OEL_HALT() to a custom action; see 'debug.h'
