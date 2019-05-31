# Obscure Efficient Library (v2)

A cross-platform, very fast substitute for C++ std::vector (and std::copy)

Unlike the containers in many of the standard libraries in use, over-aligned types (as used by SSE and AVX instructions) are supported with no special action by the user. This feature does require that you have Boost headers or a compiler supporting over-aligned dynamic allocation (C++17). If you have neither, at least compilation will fail with such types. A runtime crash would be expected using std::vector, at least with older implementations.

Features relocation optimizations similar to Folly fbvector and UnrealEngine TArray. Furthermore, OE-Lib has been optimized not only for release builds but also for execution speed in debug.

The library is distributed under the Boost Software License, and is header only, just include and go.

Visual Studio 2015 or GCC 4.8 is required. Clang has been tested briefly, minimum version is unknown.

### Visual Studio specific

For better display of dynarray and the iterators in the Visual Studio debugger, copy `oe_lib2.natvis` to:
`<My Documents>\Visual Studio 2015\Visualizers`. You can also just add `oe_lib2.natvis` to the project instead.

While usually faster than the Visual C++ standard library, performance can be an issue even for debug builds. If so, try setting inline function expansion to `/Ob1`.

### Other practical stuff

If not using Boost, you need to manually define OEL_NO_BOOST for some compilers.

To use dense matrixes, quaternions, etc. from the Eigen library efficiently in dynarray: include `optimize_ext/eigen_dense.h`

You can customize what happens when a precondition violation is detected. This is done by defining or changing OEL_ABORT or OEL_ASSERT; see `user_traits.h`
