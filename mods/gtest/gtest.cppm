module;

#include <gtest/gtest.h>

export module gtest;

export namespace testing {
using testing::AssertionResult;
using testing::InitGoogleTest;
using testing::Test;
using testing::TestInfo;
using testing::UnitTest;
} // namespace testing

export using ::RUN_ALL_TESTS;
