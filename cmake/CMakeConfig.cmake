# CPM Setup
set(CPM_USE_LOCAL_PACKAGES ON)
set(CPM_USE_NAMED_CACHE_DIRECTORIES ON)
include(cmake/get_cpm.cmake)

# Enable LTO (Link Time Optimization)
include(CheckIPOSupported)

# Optional IPO. Do not use IPO if it's not supported by compiler.
check_ipo_supported(RESULT supported OUTPUT error)
if (supported)
    message(STATUS "IPO is supported: ${supported}")
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
else ()
    message(WARNING "IPO is not supported: <${error}>")
endif ()