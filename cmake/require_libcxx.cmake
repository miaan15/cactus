include(CheckCXXSourceCompiles)
set(CMAKE_REQUIRED_FLAGS "-stdlib=libc++")
check_cxx_source_compiles("
    #include <iostream>
    int main() {
        std::cout << \"test\" << std::endl;
        return 0;
    }
" LIBCXX_AVAILABLE)

if(NOT LIBCXX_AVAILABLE)
    message(FATAL_ERROR "This requires to use libc++, libc++ is not available on this system.")
endif()

add_compile_options(-stdlib=libc++)
add_link_options(-stdlib=libc++ -lc++abi)