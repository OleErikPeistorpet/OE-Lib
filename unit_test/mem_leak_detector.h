/**
 * @file
 * @copyright (c) 2013 Stephan Brenner
 * @license   This project is released under the MIT License.
 */

#include <gtest/gtest.h>

#if _MSC_VER
#include <crtdbg.h>
#endif

class MemoryLeakDetector : public testing::EmptyTestEventListener
{
public:
    bool enabled;

#if _MSC_VER
#ifdef _DEBUG
    void OnTestStart(const testing::TestInfo&) override
    {
      _CrtMemCheckpoint(&memState_);
      enabled = true;
    }

    void OnTestEnd(const testing::TestInfo& test_info) override
	{
      if(test_info.result()->Passed() && enabled)
      {
        _CrtMemState stateNow, stateDiff;
        _CrtMemCheckpoint(&stateNow);
        int diffResult = _CrtMemDifference(&stateDiff, &memState_, &stateNow);
        if (diffResult)
        {
          FAIL() << "Memory leak of " << stateDiff.lSizes[1] << " byte(s) detected.";
        }
      }
    }

private:
    _CrtMemState memState_;
#endif // _DEBUG
#endif // _MSC_VER
};
