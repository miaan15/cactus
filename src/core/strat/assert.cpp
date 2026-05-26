module;

#include <cassert>

export module cactus.core.strat:assert;

import std;

namespace cactus {

export auto _assert(bool statement, const char *message, const std::source_location location = std::source_location::current())
    -> void {
#ifndef CACTUS_DISABLE_ASSERT
    if (statement) return;

    std::cerr << "Assertion failed: \"" << message << "\" at " << location.file_name() << ":" << location.line() << std::endl;

#if NDEBUG
    exit(1);
#else
    assert(false);
#endif
#endif
}

} // namespace cactus
