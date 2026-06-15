message(STATUS "Checking Box2D...")

set(VENDOR_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vendor")
set(BOX2D_VENDOR_SOURCE_DIR "${VENDOR_DIR}/box2d")
set(BOX2D_VENDOR_BUILD_DIR "${BOX2D_VENDOR_SOURCE_DIR}/build")

if(NOT IS_DIRECTORY "${BOX2D_VENDOR_SOURCE_DIR}")
    message(STATUS "Cloning Box2D to ${BOX2D_VENDOR_SOURCE_DIR}")
    execute_process(
        COMMAND bash -c [[
            set -e
            git clone --depth 1 --branch v3.1.1 https://github.com/erincatto/box2d.git box2d
        ]]
        WORKING_DIRECTORY "${VENDOR_DIR}"
        RESULT_VARIABLE SCRIPT_RESULT
    )
    if(NOT SCRIPT_RESULT EQUAL 0)
        message(FATAL_ERROR "Fetching Box2D failed: ${SCRIPT_RESULT}")
        file(REMOVE_RECURSE "${BOX2D_VENDOR_SOURCE_DIR}")
    endif()
endif()

if(NOT IS_DIRECTORY ${BOX2D_VENDOR_BUILD_DIR})
    message(STATUS "Building Box2D to ${BOX2D_VENDOR_BUILD_DIR}")
    execute_process(
        COMMAND bash -c [[
            set -e
            cmake -B build \
                -DCMAKE_BUILD_TYPE=Release \
                -DBUILD_SHARED_LIBS=ON \
                -DBOX2D_UNIT_TESTS=OFF \
                -DBOX2D_SAMPLES=OFF
            cmake --build build --config Release
        ]]
        WORKING_DIRECTORY "${BOX2D_VENDOR_SOURCE_DIR}"
        RESULT_VARIABLE SCRIPT_RESULT
    )
    if(NOT SCRIPT_RESULT EQUAL 0)
        message(FATAL_ERROR "Buidling failed: ${SCRIPT_RESULT}")
        file(REMOVE_RECURSE ${BOX2D_VENDOR_BUILD_DIR})
    endif()
endif()

add_library(BOX2D_VENDOR_LIB INTERFACE IMPORTED)

set(BOX2D_INCLUDE_DIR "${BOX2D_VENDOR_SOURCE_DIR}/include")
target_include_directories(BOX2D_VENDOR_LIB INTERFACE "${BOX2D_INCLUDE_DIR}")

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set_target_properties(BOX2D_VENDOR_LIB PROPERTIES
        IMPORTED_LOCATION "${SDL_VENDOR_BUILD_DIR}/Release/box2d.dll"
        IMPORTED_IMPLIB   "${SDL_VENDOR_BUILD_DIR}/Release/box2d.lib"
    )
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set_target_properties(BOX2D_VENDOR_LIB PROPERTIES
        IMPORTED_LOCATION "${SDL_VENDOR_BUILD_DIR}/libbox2d.dylib"
    )
else() 
    set_target_properties(BOX2D_VENDOR_LIB PROPERTIES
        IMPORTED_LOCATION "${SDL_VENDOR_BUILD_DIR}/libbox2d.so"
    )
endif()

set(BOX2D_TARGET "BOX2D_VENDOR_LIB")
set(BOX2D_INCLUDE_DIR ${BOX2D_INCLUDE_DIR})
set(BOX2D_SHARED_LIB_AVAILABLE ON)
set(BOX2D_STATIC_LIB_AVAILABLE OFF)

message(STATUS "Box2D Ready.")
