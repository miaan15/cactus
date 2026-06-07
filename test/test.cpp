module;
export module cactus_test;

import std;

export namespace cactus::test {

inline int fail_count = 0;

inline void check(bool condition, const std::source_location loc = std::source_location::current()) {
    if (!condition) {
        std::cerr << "[FAIL] " << loc.file_name() << ":" << loc.line() << " in " << loc.function_name() << '\n';
        fail_count++;
    }
}

template<typename T, typename U>
inline void check_eq(const T& a, const U& b, const std::source_location loc = std::source_location::current()) {
    if (!(a == b)) {
        std::cerr << "[FAIL] " << loc.file_name() << ":" << loc.line() << " in " << loc.function_name() << '\n';
        std::cerr << "       -> expected " << a << ", got " << b << '\n';
        fail_count++;
    }
}

template<typename T, typename U>
inline void check_ne(const T& a, const U& b, const std::source_location loc = std::source_location::current()) {
    if (a == b) {
        std::cerr << "[FAIL] " << loc.file_name() << ":" << loc.line() << " in " << loc.function_name() << '\n';
        std::cerr << "       -> values should not be equal: " << a << '\n';
        fail_count++;
    }
}

inline void report() {
    if (fail_count == 0) {
        std::cout << "[PASS] All tests passed\n";
    } else {
        std::cout << "[FAIL] " << fail_count << " test(s) failed\n";
    }
}

} // namespace cactus::test
