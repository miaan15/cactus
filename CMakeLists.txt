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

if(WIN32)
    message(STATUS "Platform: Windows")
elseif(APPLE)
    message(STATUS "Platform: macOS / iOS")
elseif(UNIX AND NOT APPLE)
    message(STATUS "Platform: Linux")
else()
    message(WARNING "Platform: Unknown")
endif()

set(VENDOR_DIR ${CMAKE_SOURCE_DIR}/vendor)
set(SRC_DIR "${CMAKE_SOURCE_DIR}/src")
set(DEMO_DIR "${CMAKE_SOURCE_DIR}/demo")
set(TEST_DIR "${CMAKE_SOURCE_DIR}/test")

add_subdirectory(${VENDOR_DIR}/sdl)
add_subdirectory(${VENDOR_DIR}/sdl_image)
add_subdirectory(${VENDOR_DIR}/sdl_shadercross)

if(USE_ASAN)
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer -g)
    add_link_options(-fsanitize=address)
endif()

add_subdirectory(${VENDOR_DIR}/glm)
add_subdirectory(${VENDOR_DIR}/boost)
add_subdirectory(${VENDOR_DIR}/spdlog)
add_subdirectory(${VENDOR_DIR}/tuplet)
add_subdirectory(${VENDOR_DIR}/gtest)

file(GLOB_RECURSE SRCS CONFIGURE_DEPENDS "${SRC_DIR}/*.cpp")
file(GLOB_RECURSE DEMO_SRCS CONFIGURE_DEPENDS "${DEMO_DIR}/main.cpp")
file(GLOB_RECURSE TEST_SRCS CONFIGURE_DEPENDS "${TEST_DIR}/main.cpp")

set(INC_DIRS
    ${VENDOR_DIR}/sdl/include
    ${VENDOR_DIR}/sdl_image/include
    ${VENDOR_DIR}/sdl_shadercross/include
    ${VENDOR_DIR}/boost/libs/container_hash/include
    ${VENDOR_DIR}/boost/libs/container/include
    ${VENDOR_DIR}/boost/libs/functional/include
    ${VENDOR_DIR}/boost/libs/unordered/include
    ${VENDOR_DIR}/spdlog/include
    ${VENDOR_DIR}/tuplet/include
)
set(TEST_INC_DIRS
    ${INC_DIRS}
    ${VENDOR_DIR}/gtest/googletest/include
)

set(MODS 
    ${SRCS} 
    ${VENDOR_DIR}/glm/glm/glm.cppm
)

set(LINKS
    SDL3::SDL3
    SDL3_image::SDL3_image
    SDL3_shadercross::SDL3_shadercross
    Boost::container_hash
    Boost::container
    Boost::functional
    Boost::unordered
    spdlog::spdlog
    tuplet::tuplet
)
set(TEST_LINKS
    ${LINKS}
    gtest
)

macro(create_target_demo target exe_source)
    add_executable(${target} ${exe_source})
    target_sources(${target}
        PRIVATE FILE_SET CXX_MODULES FILES ${MODS}
    )
    target_include_directories(${target} PRIVATE ${INC_DIRS})
    target_link_libraries(${target} PRIVATE ${LINKS})
endmacro()

macro(create_target_test target exe_source)
    add_executable(${target} ${exe_source})
    target_sources(${target}
        PRIVATE FILE_SET CXX_MODULES FILES ${MODS}
    )
    target_include_directories(${target} PRIVATE ${TEST_INC_DIRS})
    target_link_libraries(${target} PRIVATE ${TEST_LINKS})
endmacro()

foreach(source IN LISTS DEMO_SRCS) 
    get_filename_component(parent_dir "${source}" DIRECTORY)
    get_filename_component(folder "${parent_dir}" NAME)
    set(target "${folder}_demo")
    create_target_demo(${target} ${source})
endforeach()

foreach(source IN LISTS TEST_SRCS) 
    get_filename_component(parent_dir "${source}" DIRECTORY)
    get_filename_component(folder "${parent_dir}" NAME)
    set(target "${folder}_test")
    create_target_test(${target} ${source})
endforeach()
