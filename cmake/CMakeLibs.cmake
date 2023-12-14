# Range-v3
find_package(range-v3 REQUIRED)
target_link_libraries(${PROJECT_NAME}_${PROJECT_NAME} INTERFACE range-v3::range-v3)

# tabulate
find_package(tabulate REQUIRED)
target_link_libraries(${PROJECT_NAME}_${PROJECT_NAME} INTERFACE tabulate::tabulate)

# prox
find_package(prox REQUIRED)
target_link_libraries(${PROJECT_NAME}_${PROJECT_NAME} INTERFACE prox::prox)

# libnuma
target_link_libraries(${PROJECT_NAME}_${PROJECT_NAME} INTERFACE numa)