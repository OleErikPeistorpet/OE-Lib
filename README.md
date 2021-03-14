# Obscure Efficient Library (v2)

A cross-platform, very fast substitute for C++ std::vector (and std::copy)

Features relocation optimizations similar to [Folly fbvector](https://github.com/facebook/folly/blob/master/folly/docs/FBVector.md#object-relocation) and UnrealEngine TArray.

Unlike the containers in many older standard library implementations, over-aligned types (as used by SSE and AVX instructions) are supported with no special action by the user. A runtime crash would often be the result with std::vector.

The library is distributed under the Boost Software License, and is header only, just include and go.

Supported compilers:
* Visual Studio 2015 and later
* GCC 5 and later
* Clang is tested regularly (Travis CI), but minimum version is unknown

### Checked preconditions

Precondition checks are off by default except for Visual C++ debug builds. (Preconditions are the same as std::vector.) They can be controlled with a global define such as `-D OEL_MEM_BOUND_DEBUG_LVL=2`. But be careful with compilers other than MSVC, they should **not** be combined with compiler optimizations unless you set the `-fno-strict-aliasing` flag.

You can customize what happens when a check is triggered. This is done by defining or changing OEL_ABORT or OEL_ASSERT; see `user_traits.h`

### Visual Studio specific

For better display of dynarray and the iterators in the Visual Studio debugger, copy `oe_lib2.natvis` to:
`<My Documents>\Visual Studio 2015\Visualizers`. You can also just add `oe_lib2.natvis` to the project instead.

While usually faster than the Visual C++ standard library, performance can be an issue even for debug builds. If so, try setting inline function expansion to `/Ob1`, Basic Runtime Checks to default, and/or an environment variable `_NO_DEBUG_HEAP=1`.

### Other

If not using Boost, you need to manually define OEL_NO_BOOST for some older compilers.

To use dense matrixes, quaternions, etc. from the Eigen library efficiently in dynarray: include `optimize_ext/eigen_dense.h`
