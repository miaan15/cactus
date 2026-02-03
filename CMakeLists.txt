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

set(VENDOR_DIR ${CMAKE_SOURCE_DIR}/vendor)
set(SRC_DIR "${CMAKE_SOURCE_DIR}/src")

add_subdirectory(${VENDOR_DIR}/boost)
add_subdirectory(${VENDOR_DIR}/raylib)
add_subdirectory(${VENDOR_DIR}/glm)

add_executable(app ${SRC_DIR}/main.cpp)

file(GLOB_RECURSE SRCS CONFIGURE_DEPENDS "${SRC_DIR}/*.cpp")
list(REMOVE_ITEM SRCS
    ${SRC_DIR}/main.cpp
)

target_sources(app
    PUBLIC
    FILE_SET CXX_MODULES FILES
        ${SRCS}
)
target_sources(app
    PUBLIC
    FILE_SET CXX_MODULES FILES
        ${VENDOR_DIR}/glm/glm/glm.cppm
)

target_include_directories(app PRIVATE
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
)

target_link_libraries(app PRIVATE 
    raylib
    Boost::container
)

