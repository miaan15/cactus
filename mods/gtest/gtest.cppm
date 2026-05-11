export module gtest;

// Bring in gtest declarations and re-export them
#include <gtest/gtest.h>

// Export the key parts of the testing namespace
export namespace testing {
    using InitGoogleTest = void(*)(int* argc, char** argv);
    auto InitGoogleTest(int* argc, char** argv) -> void;

    class Test;
    class TestInfo;
    class TestSuite;
    class UnitTest;
}

// Export main test runner
export auto RUN_ALL_TESTS() -> int;
