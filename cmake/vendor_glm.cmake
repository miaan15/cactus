message(STATUS "Checking glm...")

set(VENDOR_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vendor")
set(GLM_VENDOR_SOURCE_DIR "${VENDOR_DIR}/glm")

if(NOT IS_DIRECTORY "${GLM_VENDOR_SOURCE_DIR}")
    message(STATUS "Cloning glm to ${GLM_VENDOR_SOURCE_DIR}")
    execute_process(
        COMMAND bash -c [[
            set -e
            git clone --depth 1 --branch 1.0.3 https://github.com/g-truc/glm.git glm
        ]]
        WORKING_DIRECTORY "${VENDOR_DIR}"
        RESULT_VARIABLE SCRIPT_RESULT
    )
    if(NOT SCRIPT_RESULT EQUAL 0)
        message(FATAL_ERROR "Fetching glm failed: ${SCRIPT_RESULT}")
        file(REMOVE_RECURSE "${GLM_VENDOR_SOURCE_DIR}")
    endif()
endif()

add_library(GLM_VENDOR_LIB INTERFACE IMPORTED)

set(GLM_INCLUDE_DIR "${GLM_VENDOR_SOURCE_DIR}")
target_include_directories(GLM_VENDOR_LIB INTERFACE "${GLM_INCLUDE_DIR}")

set(GLM_TARGET "GLM_VENDOR_LIB")

set(GLM_TARGET ${GLM_TARGET})
set(GLM_INCLUDE_DIR ${GLM_INCLUDE_DIR})
set(GLM_MODULE_SOURCE "${GLM_VENDOR_SOURCE_DIR}/glm/glm.cppm")

message(STATUS "glm Ready.")
