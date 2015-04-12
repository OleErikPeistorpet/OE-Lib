# OE-Lib
A very fast substitute for C++ std::vector (and std::copy)

The library is header only, just include and go.

The main class, dynarray, supports over-aligned types (as used by AVX instructions) if you have boost headers
(1.56 or newer).

For better display of dynarray and the iterators in the Visual Studio debugger, copy `oe_lib.natvis` to:
`<My Documents>\Visual Studio 2013\Visualizers` (should also work for newer Visual Studio, just different year in path)

To use dense matrixes, quaternions, etc. from the Eigen library in dynarray: include `compat/eigen_dense.h`
