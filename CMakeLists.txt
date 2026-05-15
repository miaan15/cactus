# Only support Clang and libc++ (for now, till all support c++ modules) // TODO

cmake_minimum_required(VERSION 3.29)

# Check Clang
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/require_clang.cmake)

project(cactus LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# Check libc++
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/require_libcxx.cmake)

# ---------------------------------------------------------
# Options
# ---------------------------------------------------------
# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type" FORCE)
endif()

# Asan
option(USE_ASAN "Enable AddressSanitizer" OFF)

# Lib type
option(BUILD_SHARED_LIBS "Build as shared/dynamic library" OFF)

# Test
option(CACTUS_BUILD_TESTS "Build the unit tests" OFF)

# ---------------------------------------------------------
# Platforms
# ---------------------------------------------------------
if(WIN32)
    message(STATUS "Platform: Windows")
elseif(APPLE)
    message(STATUS "Platform: macOS / iOS")
elseif(UNIX AND NOT APPLE)
    message(STATUS "Platform: Linux")
else()
    message(WARNING "Platform: Unknown")
endif()

# ---------------------------------------------------------
# Fetching
# ---------------------------------------------------------
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

# ---------------------------------------------------------
# Vars
# ---------------------------------------------------------
set(VENDOR_DIR ${CMAKE_SOURCE_DIR}/vendor)
set(MODULE_DIR ${CMAKE_SOURCE_DIR}/modules)
set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)
set(DEMO_DIR ${CMAKE_SOURCE_DIR}/demo)
set(TEST_DIR ${CMAKE_SOURCE_DIR}/test)
set(INC_DIRS
    ${VENDOR_DIR}/raylib/src
    ${VENDOR_DIR}/spdlog/include
)
file(GLOB_RECURSE SRCS CONFIGURE_DEPENDS "${SRC_DIR}/*.cpp")
file(GLOB_RECURSE SRC_MODULES CONFIGURE_DEPENDS "${SRC_DIR}/*.cppm")

# ---------------------------------------------------------
# Flags
# ---------------------------------------------------------
if(USE_ASAN)
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer -g)
    add_link_options(-fsanitize=address)
endif()

add_compile_options(-ftime-trace)

# ---------------------------------------------------------
# Create target
# ---------------------------------------------------------
# Modules
set(MODULE_FILES
    ${SRCS}
    ${SRC_MODULES}
    ${MODULE_DIR}/std/std.cppm
    ${MODULE_DIR}/raylib/raylib.cppm
    ${VENDOR_DIR}/glm/glm/glm.cppm
    ${MODULE_DIR}/spdlog/spdlog.cppm
)
set(LIBS
    # TODO
)

# Build cactus
add_library(cactus ${SRCS})

target_sources(cactus PUBLIC 
    FILE_SET CXX_MODULES FILES ${MODULE_FILES}
)

target_include_directories(cactus PUBLIC ${INC_DIRS})
target_link_libraries(cactus PUBLIC ${LIBS})

# ---------------------------------------------------------
# Test
# ---------------------------------------------------------
if(CACTUS_BUILD_TESTS)
    enable_testing()

    function(create_target_test TEST_NAME TEST_SOURCE)
        add_executable(${TEST_NAME} ${TEST_SOURCE})

        set_target_properties(${TEST_NAME} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/test"
        )

        target_link_libraries(${TEST_NAME} PRIVATE
            cactus
        )

        target_include_directories(${TEST_NAME} PRIVATE 
            "${VENDOR_DIR}/doctest/doctest"
        )

        add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
    endfunction()

    file(GLOB_RECURSE TEST_SRCS CONFIGURE_DEPENDS "${TEST_DIR}/*.cpp")
    foreach(TEST_SOURCE IN LISTS TEST_SRCS) 
        get_filename_component(TEST_SOURCE_NAME "${TEST_SOURCE}" NAME_WE)
        set(TEST_NAME "${TEST_SOURCE_NAME}_test")

        create_target_test(${TEST_NAME} ${TEST_SOURCE})
    endforeach()
endif()
