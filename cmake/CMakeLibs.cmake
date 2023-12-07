# Range-v3
find_package(range-v3 CONFIG REQUIRED)
if (TARGET range-v3::range-v3)
    message(STATUS "Found range-v3: OK")
    target_link_libraries(${PROJECT_NAME}_${PROJECT_NAME} INTERFACE range-v3::range-v3)
else ()
    message(SEND_ERROR "Found range-v3: ERROR")
endif ()

# fmt
find_package(fmt CONFIG REQUIRED)
if (TARGET fmt::fmt)
    message(STATUS "Found fmt: OK")
    target_link_libraries(${PROJECT_NAME}_${PROJECT_NAME} INTERFACE fmt::fmt)
else ()
    message(SEND_ERROR "Found fmt: ERROR")
endif ()

# tabulate
find_package(tabulate CONFIG REQUIRED)
if (TARGET tabulate::tabulate)
    message(STATUS "Found tabulate: OK")
    target_link_libraries(${PROJECT_NAME}_${PROJECT_NAME} INTERFACE tabulate::tabulate)
else ()
    message(SEND_ERROR "Found tabulate: ERROR")
endif ()

# prox
find_package(prox CONFIG REQUIRED)
if (TARGET prox::prox)
    message(STATUS "Found prox: OK")
    target_link_libraries(${PROJECT_NAME}_${PROJECT_NAME} INTERFACE prox::prox)
else ()
    message(SEND_ERROR "Found prox: ERROR")
endif ()

# libnuma
target_link_libraries(${PROJECT_NAME}_${PROJECT_NAME} INTERFACE numa)