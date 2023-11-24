# CPM Setup
set(CPM_USE_LOCAL_PACKAGES ON)
set(CPM_USE_NAMED_CACHE_DIRECTORIES ON)
include(cmake/get_cpm.cmake)

# Add our module directory to the include path.
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake;${CMAKE_MODULE_PATH}")

include_directories(include/)