cmake_minimum_required(VERSION 3.14)

set(namespace syssnap)

# Set the project name and language
project(syssnap
        VERSION 1.0
        DESCRIPTION "Library to get snapshots about the processes running in the system."
        LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(cmake/project-is-top-level.cmake)
include(cmake/variables.cmake)

message(STATUS "Searching targets")
include(cmake/CMakeTargets.cmake)
message(STATUS "Searching targets - done")

message(STATUS "Searching external libraries")
include(cmake/CMakeLibs.cmake)
message(STATUS "Searching external libraries - done")

message(STATUS "Configuring install")
if (NOT CMAKE_SKIP_INSTALL_RULES)
    include(cmake/install-rules.cmake)
endif ()
message(STATUS "Configuring install - done")

# ---- Examples ----

option(BUILD_EXAMPLES "Build example(s)" OFF)
if(PROJECT_IS_TOP_LEVEL)
    set(BUILD_EXAMPLES "${${PROJECT_NAME}_DEVELOPER_MODE}")
endif()

if(BUILD_EXAMPLES)
    add_subdirectory(example)
endif()

# ---- Developer mode ----

if (NOT ${PROJECT_NAME}_DEVELOPER_MODE)
    return()
elseif (NOT PROJECT_IS_TOP_LEVEL)
    message(
            AUTHOR_WARNING
            "Developer mode is intended for developers of ${PROJECT_NAME}"
    )
endif ()

include(cmake/dev-mode.cmake)