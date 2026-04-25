cmake_minimum_required(VERSION 4.1.4)

project(CACTUS LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

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
set(SRC_DIR "${CMAKE_SOURCE_DIR}/src")
set(DEMO_DIR "${CMAKE_SOURCE_DIR}/demo")
set(TEST_DIR "${CMAKE_SOURCE_DIR}/test")

if(USE_ASAN)
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer -g)
    add_link_options(-fsanitize=address)
endif()
