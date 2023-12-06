cmake_minimum_required(VERSION 3.14)

project(proxTests LANGUAGES CXX)

include(../cmake/project-is-top-level.cmake)
include(../cmake/folders.cmake)

# ---- Dependencies ----

if (PROJECT_IS_TOP_LEVEL)
    find_package(syssnap REQUIRED)
    enable_testing()
endif ()

# ---- Tests ----

# Find package for GTest
find_package(GTest REQUIRED)

# Get the names of all the tests from the source files
file(GLOB TEST_SOURCES CONFIGURE_DEPENDS source/*.cpp)
foreach (testSource ${TEST_SOURCES})
    get_filename_component(testName ${testSource} NAME_WE)
    list(APPEND TEST_NAMES ${testName})
endforeach ()

# Add a test for each test file
foreach (testName ${TEST_NAMES})
    add_executable(${testName} source/${testName}.cpp)

    target_include_directories(${testName} PRIVATE include)

    target_link_libraries(${testName} PRIVATE syssnap::syssnap GTest::GTest GTest::Main GTest::gmock_main)

    target_compile_features(${testName} PRIVATE cxx_std_20)

    add_test(NAME ${testName} COMMAND ${testName})
endforeach ()

# ---- End-of-file commands ----

add_folders(Test)
