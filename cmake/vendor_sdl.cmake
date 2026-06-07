message(STATUS "Checking SDL3...")

set(VENDOR_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vendor")
set(SDL_VENDOR_SOURCE_DIR "${VENDOR_DIR}/sdl")
set(SDL_VENDOR_BUILD_DIR "${SDL_VENDOR_SOURCE_DIR}/build")

set(USE_SYSTEM_SDL3 OFF)

if(NOT IS_DIRECTORY ${SDL_VENDOR_SOURCE_DIR})
    message(STATUS "Finding SDL3...")
    find_package(SDL3 QUIET)

    if(SDL3_FOUND)
        message(STATUS "Found system SDL3; use system SDL3.")
        set(USE_SYSTEM_SDL3 ON)

        if(TARGET SDL3::SDL3-shared)
            set(SDL3_TARGET "SDL3::SDL3-shared")
            set(SDL3_SHARED_LIB_AVAILABLE ON)
            set(SDL3_STATIC_LIB_AVAILABLE OFF)
        else()
            set(SDL3_TARGET "SDL3::SDL3-static")
            set(SDL3_SHARED_LIB_AVAILABLE OFF)
            set(SDL3_STATIC_LIB_AVAILABLE ON)
        endif()

        get_target_property(SDL3_INCLUDE_DIR ${SDL3_TARGET} INTERFACE_INCLUDE_DIRECTORIES)

    else()
        message(STATUS "Cloning SDL3 to ${SDL_VENDOR_SOURCE_DIR}")
        execute_process(
            COMMAND bash -c [[
                set -e
                git clone --depth 1 --branch release-3.4.10 https://github.com/libsdl-org/SDL.git sdl
            ]]
            WORKING_DIRECTORY "${VENDOR_DIR}"
            RESULT_VARIABLE SCRIPT_RESULT
        )
        if(NOT SCRIPT_RESULT EQUAL 0)
            message(FATAL_ERROR "Fetching failed: ${SCRIPT_RESULT}")
            file(REMOVE_RECURSE ${SDL_VENDOR_SOURCE_DIR})
        endif()

    endif()
endif()

if (NOT USE_SYSTEM_SDL3)
    if(NOT IS_DIRECTORY ${SDL_VENDOR_BUILD_DIR}) 
        message(STATUS "Building SDL3 to ${SDL_VENDOR_BUILD_DIR}")
        execute_process(
            COMMAND bash -c [[
                set -e
                cmake -B build \
                    -DCMAKE_BUILD_TYPE=Release \
                    -DSDL_SHARED=ON \
                    -DSDL_STATIC=OFF \
                    -DSDL_TESTS=OFF
                cmake --build build --config Release
            ]]
            WORKING_DIRECTORY "${SDL_VENDOR_SOURCE_DIR}"
            RESULT_VARIABLE SCRIPT_RESULT
        )
        if(NOT SCRIPT_RESULT EQUAL 0)
            message(FATAL_ERROR "Buidling failed: ${SCRIPT_RESULT}")
            file(REMOVE_RECURSE ${SDL_VENDOR_BUILD_DIR})
        endif()
    endif()

    add_library(SDL3_VENDOR_LIB SHARED IMPORTED)

    set(SDL3_INCLUDE_DIR "${SDL_VENDOR_SOURCE_DIR}/include")
    target_include_directories(SDL3_VENDOR_LIB INTERFACE "${SDL3_INCLUDE_DIR}")

    if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
        set_target_properties(SDL3_VENDOR_LIB PROPERTIES
            IMPORTED_LOCATION "${SDL_VENDOR_BUILD_DIR}/Release/SDL3.dll"
            IMPORTED_IMPLIB   "${SDL_VENDOR_BUILD_DIR}/Release/SDL3.lib"
        )
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
        set_target_properties(SDL3_VENDOR_LIB PROPERTIES
            IMPORTED_LOCATION "${SDL_VENDOR_BUILD_DIR}/libSDL3.dylib"
        )
    else() 
        set_target_properties(SDL3_VENDOR_LIB PROPERTIES
            IMPORTED_LOCATION "${SDL_VENDOR_BUILD_DIR}/libSDL3.so"
        )
    endif()

    set(SDL3_TARGET "SDL3_VENDOR_LIB")

    set(SDL3_SHARED_LIB_AVAILABLE ON)
    set(SDL3_STATIC_LIB_AVAILABLE OFF)

endif()

set(SDL_TARGET ${SDL3_TARGET})
set(SDL_INCLUDE_DIR ${SDL3_INCLUDE_DIR})
set(SDL_SHARED_LIB_AVAILABLE ${SDL3_SHARED_LIB_AVAILABLE})
set(SDL_STATIC_LIB_AVAILABLE ${SDL3_STATIC_LIB_AVAILABLE})

message(STATUS "SDL3 Ready.")
