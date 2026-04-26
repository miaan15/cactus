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

# Fetch
message(STATUS "Running vendor fetch script...")
set(FETCH_VENDORS_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/fetch_vendors.sh)
file(CHMOD "${FETCH_VENDORS_SCRIPT}"
    PERMISSIONS
    OWNER_READ OWNER_WRITE OWNER_EXECUTE
    GROUP_READ GROUP_EXECUTE
    WORLD_READ WORLD_EXECUTE
)
execute_process(
    COMMAND "${FETCH_VENDORS_SCRIPT}"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

# Set vars
set(VENDOR_DIR ${CMAKE_SOURCE_DIR}/vendor)
set(MODS_DIR ${CMAKE_SOURCE_DIR}/mods)
set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)
set(DEMO_DIR ${CMAKE_SOURCE_DIR}/demo)
set(TEST_DIR ${CMAKE_SOURCE_DIR}/test)
file(GLOB_RECURSE SRCS CONFIGURE_DEPENDS "${SRC_DIR}/*.cpp")
set(INC_DIRS
    ${VENDOR_DIR}/raylib/src
    ${VENDOR_DIR}/spdlog/include
)

if(USE_ASAN)
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer -g)
    add_link_options(-fsanitize=address)
endif()

# Set modules
set(MOD_SOURCES 
    ${SRCS}
    ${MODS_DIR}/std/std.cppm
    ${MODS_DIR}/raylib/raylib.cppm
    ${VENDOR_DIR}/glm/glm/glm.cppm
    ${MODS_DIR}/spdlog/spdlog.cppm
)

# Finalize
add_executable(cactus_demo_main ${DEMO_DIR}/main.cpp)
target_sources(cactus_demo_main PRIVATE FILE_SET CXX_MODULES FILES ${MOD_SOURCES})
target_include_directories(cactus_demo_main PRIVATE ${INC_DIRS})
