# Try to use Clang
find_program(CLANG_C_COMPILER NAMES clang)
find_program(CLANG_CXX_COMPILER NAMES clang++)

if(CLANG_C_COMPILER AND CLANG_CXX_COMPILER)
    # Force set the compiler
    set(CMAKE_C_COMPILER ${CLANG_C_COMPILER} CACHE STRING "c compiler" FORCE)
    set(CMAKE_CXX_COMPILER ${CLANG_CXX_COMPILER} CACHE STRING "cxx compiler" FORCE)
else()
    message(FATAL_ERROR "This requires to use Clang, Clang compiler not found on this system.")
endif()