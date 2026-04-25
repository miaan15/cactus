# Only support Clang and libc++ (for now, till all support c++ mods) // TODO
cmake_minimum_required(VERSION 3.28)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/require_clang.cmake)

project(cactus LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/require_libcxx.cmake)

# Options
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type" FORCE)
endif()

option(USE_ASAN "Enable AddressSanitizer" OFF)

# Platforms
if(WIN32)
    message(STATUS "Platform: Windows")
elseif(APPLE)
    message(STATUS "Platform: macOS / iOS")
elseif(UNIX AND NOT APPLE)
    message(STATUS "Platform: Linux")
else()
    message(WARNING "Platform: Unknown")
endif()

# Set vars
set(VENDOR_DIR ${CMAKE_SOURCE_DIR}/vendor)
set(MODS_DIR ${CMAKE_SOURCE_DIR}/mods)
set(SRC_DIR "${CMAKE_SOURCE_DIR}/src")
set(DEMO_DIR "${CMAKE_SOURCE_DIR}/demo")
set(TEST_DIR "${CMAKE_SOURCE_DIR}/test")
file(GLOB_RECURSE SRCS CONFIGURE_DEPENDS "${SRC_DIR}/*.cpp")

if(USE_ASAN)
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer -g)
    add_link_options(-fsanitize=address)
endif()

# Create precompiled modules
macro(create_precompiled_mod target)
    set(MOD_TARGET ${target}_MOD)
    set(MOD_SOURCE "${MODS_DIR}/${target}/${target}.cppm")

    if(NOT EXISTS "${MOD_SOURCE}")
        message(FATAL_ERROR "Module file not found: ${MOD_SOURCE}")
    endif()

    add_library(${MOD_TARGET} STATIC)
    target_sources(${MOD_TARGET} PUBLIC FILE_SET CXX_MODULES FILES "${MOD_SOURCE}")

    set_target_properties(${MOD_TARGET} PROPERTIES
        CXX_STANDARD 23
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
    )

    target_compile_options(${MOD_TARGET} PUBLIC -stdlib=libc++)
    target_link_options(${MOD_TARGET} PUBLIC -stdlib=libc++)
endmacro()


# Set modules
set(MOD_SOURCES ${SRCS})

create_precompiled_mod(std)
list(APPEND MOD_LIBS ${MOD_TARGET})

# Finalize
add_executable(cactus_demo_main ${DEMO_DIR}/main.cpp)
target_sources(cactus_demo_main PRIVATE FILE_SET CXX_MODULES FILES ${MOD_SOURCES})
target_link_libraries(cactus_demo_main PRIVATE ${MOD_LIBS})
