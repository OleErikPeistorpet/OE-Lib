# OE-Lib

A cross-platform, very fast substitute for C++ std::vector with a range-based interface, and a bunch of supporting utilities.

Features relocation optimizations similar to [Folly fbvector](https://github.com/facebook/folly/blob/master/folly/docs/FBVector.md#object-relocation) and UnrealEngine TArray. Also uses `realloc` when possible, which showed substantial performance improvements when tested on a Linux system.

The library is distributed under the Boost Software License, and is header only, just include and go.

C++17 is required. Oldest supported compilers:
* Visual Studio 2017 (15.8)
* GCC 7
* Clang 5

### Append

You should use the `append` member function of dynarray instead of doing `insert` of multiple elements at the end. This is often substantially faster than with std::vector (likely due to inlining which lets the compiler optimize further). Example:

	void moveAllToBackOf(std::vector<Foo> & dest)
	{
		dest.insert(dest.end(), std::move_iterator{fooList.begin()}, std::move_iterator{fooList.end()});
	}

Would be better as:

	void moveAllToBackOf(oel::dynarray<Foo> & dest)
	{
		dest.append(fooList | oel::view::move);
	}

Compared to calling `push_back` or `emplace_back` in a loop, `append` has major benefits, mainly because of fewer memory allocations without having to worry about manual `reserve`. Moreover, calling `reserve` inside a loop is a performance pitfall that many aren't aware of. For example, see here: <https://stackoverflow.com/questions/48535727/why-are-c-stl-vectors-1000x-slower-when-doing-many-reserves>

	for (int i{}; i < outerLimit; i++)
	{
		arr.reserve(arr.size() + innerLimit); // typically BAD inside loop, but easy to miss in more complex code
		for (int j{}; j < innerLimit; j++)
			arr.push_back(i * j);
	}

Should be something like:

	for (int i{}; i < outerLimit; i++)
	{
		auto fn = [i, j = 0]() mutable { return i * j++; };
		arr.append(oel::view::generate(fn, innerLimit));
	}

Another good way, using `resize_for_overwrite`:

	std::size_t arrIdx{};
	for (int i{}; i < outerLimit; i++)
	{
		arr.resize_for_overwrite(arr.size() + innerLimit);
		for (int j{}; j < innerLimit; j++)
			arr[arrIdx++] = i * j;
	}

### Checked preconditions

Precondition checks are off by default except for Visual C++ debug builds. (Preconditions are the same as std::vector except a few documented cases.) They can be controlled with a global define such as `-D OEL_MEM_BOUND_DEBUG_LVL=2`. But be careful with compilers other than MSVC, the checks should **not** be combined with compiler optimizations unless you set the `-fno-strict-aliasing` flag.

You can customize what happens when a check is triggered. This is done by defining or changing OEL_ASSERT; see `fwd.h`

### Visual Studio visualizer

For better display of dynarray and the iterators in the Visual Studio debugger, copy `oe_lib3.natvis` to:
`Documents\Visual Studio 20**\Visualizers`. You can also just add `oe_lib3.natvis` to the project instead.
