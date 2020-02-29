/**
 * This file implements a main() function for Google Test that runs all tests
 * and detects memory leaks.
 */

#include "mem_leak_detector.h"

#include <iostream>
#include <gtest/gtest.h>

MemoryLeakDetector* leakDetector;

GTEST_API_ int main(int argc, char **argv)
{
  std::cout << "Running main() from gtest_mem_main.cpp" << std::endl;

  testing::InitGoogleTest(&argc, argv);
  leakDetector = new MemoryLeakDetector();
#if _MSC_VER
  testing::UnitTest::GetInstance()->listeners().Append(leakDetector);
#endif

  return RUN_ALL_TESTS();
}