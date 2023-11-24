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

message(STATUS "Configuring CMake variables")
include(cmake/CMakePreConf.cmake)
message(STATUS "Configuring CMake variables - done")

message(STATUS "Searching external libraries")
include(cmake/CMakeLibs.cmake)
message(STATUS "Searching external libraries - done")

message(STATUS "Linking libraries")
include(cmake/CMakeLink.cmake)
message(STATUS "Linking libraries - done")

message(STATUS "Configuring install")
if (NOT CMAKE_SKIP_INSTALL_RULES)
    include(cmake/CMakeInstall.cmake)
endif ()
message(STATUS "Configuring install - done")

# ---- Developer mode ----

if (NOT DEVELOPER_MODE)
    return()
elseif (NOT PROJECT_IS_TOP_LEVEL)
    message(
            AUTHOR_WARNING
            "Developer mode is intended for developers of ${PROJECT_NAME}"
    )
endif ()