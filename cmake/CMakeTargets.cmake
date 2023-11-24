file(GLOB_RECURSE sources CONFIGURE_DEPENDS src/*.cpp)
file(GLOB_RECURSE public_headers CONFIGURE_DEPENDS include/system_snapshot/*.hpp)

add_library(${PROJECT_NAME} INTERFACE "${public_headers}")
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

set_property(TARGET ${PROJECT_NAME} PROPERTY EXPORT_NAME ${PROJECT_NAME})

target_sources(${PROJECT_NAME} PUBLIC "${sources}")

target_include_directories(${PROJECT_NAME}
        INTERFACE
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
)

target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_20)