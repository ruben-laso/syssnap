cmake_minimum_required(VERSION 3.14)

project(syssnapExamples CXX)

include(../cmake/project-is-top-level.cmake)
include(../cmake/folders.cmake)

if (PROJECT_IS_TOP_LEVEL)
    find_package(syssnap REQUIRED)
endif ()

find_package(prox REQUIRED)
find_package(spdlog REQUIRED)
find_package(range-v3 REQUIRED)
find_package(CLI11 REQUIRED)

add_custom_target(run-examples)

function(add_example NAME)
    add_executable("${NAME}" "${NAME}.cpp")
    target_link_libraries("${NAME}" PRIVATE prox::prox)
    target_link_libraries("${NAME}" PRIVATE syssnap::syssnap)
    target_link_libraries("${NAME}" PRIVATE spdlog::spdlog)
    target_link_libraries("${NAME}" PRIVATE CLI11::CLI11)
    target_link_libraries("${NAME}" PRIVATE range-v3::range-v3)
    target_compile_features("${NAME}" PRIVATE cxx_std_20)
    add_custom_target("run_${NAME}" COMMAND "${NAME}" VERBATIM)
    add_dependencies("run_${NAME}" "${NAME}")
    add_dependencies(run-examples "run_${NAME}")
endfunction()

add_example(example)

add_folders(Example)
