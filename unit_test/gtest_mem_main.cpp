// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/*
 * This file implements a main() function for Google Test that runs all tests
 * and detects memory leaks.
 */

#include "mem_leak_detector.h"

#include <gtest/gtest.h>
#include <cstdio>

MemoryLeakDetector* leakDetector;

GTEST_API_ int main(int argc, char** argv)
{
	std::puts("Running main() from gtest_mem_main.cpp");

	testing::InitGoogleTest(&argc, argv);
	leakDetector = new MemoryLeakDetector();
#if _MSC_VER
	testing::UnitTest::GetInstance()->listeners().Append(leakDetector);
#endif

	return RUN_ALL_TESTS();
}