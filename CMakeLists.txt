cmake_minimum_required(VERSION 4.1.4)

project(CACTUS LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type" FORCE)
endif()

option(USE_ASAN "Enable AddressSanitizer" OFF)
if(USE_ASAN)
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer -g)
    add_link_options(-fsanitize=address)
endif()

set(VENDOR_DIR ${CMAKE_SOURCE_DIR}/vendor)
set(SRC_DIR "${CMAKE_SOURCE_DIR}/src")
set(TEST_DIR "${CMAKE_SOURCE_DIR}/test")

add_subdirectory(${VENDOR_DIR}/boost)
add_subdirectory(${VENDOR_DIR}/raylib)
add_subdirectory(${VENDOR_DIR}/glm)
add_subdirectory(${VENDOR_DIR}/gtest)

file(GLOB_RECURSE SRCS CONFIGURE_DEPENDS "${SRC_DIR}/*.cpp")
list(REMOVE_ITEM SRCS
    ${SRC_DIR}/main.cpp
)

file(GLOB_RECURSE TEST_SRCS CONFIGURE_DEPENDS "${TEST_DIR}/main.cpp")

set(INC_DIRS
    ${VENDOR_DIR}/raylib/src

    ${VENDOR_DIR}/boost/libs/core/include
    ${VENDOR_DIR}/boost/libs/config/include
    ${VENDOR_DIR}/boost/libs/assert/include
    ${VENDOR_DIR}/boost/libs/static_assert/include
    ${VENDOR_DIR}/boost/libs/type_traits/include
    ${VENDOR_DIR}/boost/libs/throw_exception/include
    ${VENDOR_DIR}/boost/libs/move/include
    ${VENDOR_DIR}/boost/libs/intrusive/include
    ${VENDOR_DIR}/boost/libs/container/include

    ${VENDOR_DIR}/gtest/googletest/include
)

set(LINKS
    raylib
    Boost::container
    gtest
)

macro(create_target target exe_source)
    add_executable(${target} ${exe_source})
    target_sources(${target}
        PRIVATE
        FILE_SET CXX_MODULES FILES
            ${SRCS}
            ${VENDOR_DIR}/glm/glm/glm.cppm
    )
    target_include_directories(${target} PRIVATE ${INC_DIRS})
    target_link_libraries(${target} PRIVATE ${LINKS})
endmacro()

create_target(main ${SRC_DIR}/main.cpp)

foreach(source IN LISTS TEST_SRCS) 
    get_filename_component(parent_dir "${source}" DIRECTORY)
    get_filename_component(folder "${parent_dir}" NAME)
    set(target "${folder}_test")
    create_target(${target} ${source})
endforeach()
