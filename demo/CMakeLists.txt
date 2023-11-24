cmake_minimum_required(VERSION 3.14)

project("system_snapshot_demo" CXX)

set(CMAKE_CXX_STANDARD 20)

add_executable(${PROJECT_NAME} main.cpp)

set(CPM_USE_LOCAL_PACKAGES ON)
set(CPM_USE_NAMED_CACHE_DIRECTORIES ON)
include(../cmake/get_cpm.cmake)

# spdlog
CPMAddPackage("gh:gabime/spdlog")
if (TARGET spdlog::spdlog)
    message(STATUS "Found spdlog: OK")
else ()
    message(SEND_ERROR "Found spdlog: ERROR")
endif ()

# Range-v3
CPMAddPackage("gh:ericniebler/range-v3@0.12.0")
if (TARGET range-v3::range-v3)
    message(STATUS "Found range-v3: OK")
else ()
    message(SEND_ERROR "Found range-v3: ERROR")
endif ()

# CLI11
CPMAddPackage("gh:CLIUtils/CLI11@2.3.2")
if (TARGET CLI11::CLI11)
    message(STATUS "Found CLI11: OK")
else ()
    message(SEND_ERROR "Found CLI11: ERROR")
endif ()

# syssnap
find_package(syssnap REQUIRED)
if (syssnap_FOUND)
    message(STATUS "Found syssnap: OK")
else ()
    message(FATAL_ERROR "Found syssnap: ERROR")
endif ()


if (TARGET syssnap::syssnap)
    target_link_libraries(${PROJECT_NAME} PRIVATE syssnap::syssnap)
endif ()

if (TARGET spdlog::spdlog)
    target_link_libraries(${PROJECT_NAME} PRIVATE spdlog::spdlog)
endif ()

if (TARGET range-v3::range-v3)
    target_link_libraries(${PROJECT_NAME} PRIVATE range-v3::range-v3)
endif ()

if (TARGET CLI11::CLI11)
    target_link_libraries(${PROJECT_NAME} PRIVATE CLI11::CLI11)
endif ()