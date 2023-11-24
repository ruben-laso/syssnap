if (PROJECT_IS_TOP_LEVEL)
    set(
            CMAKE_INSTALL_INCLUDEDIR "include/${PROJECT_NAME}-${PROJECT_VERSION}"
            CACHE PATH ""
    )
endif ()

# Project is configured with no languages, so tell GNUInstallDirs the lib dir
set(CMAKE_INSTALL_LIBDIR lib CACHE PATH "")

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

# find_package(<package>) call for consumers to find this project
set(package ${PROJECT_NAME})

install(
        DIRECTORY include/
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
        COMPONENT ${PROJECT_NAME}_Development
)

install(
        TARGETS ${PROJECT_NAME}
        EXPORT ${PROJECT_NAME}Targets
        INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)

write_basic_package_version_file(
        "${package}ConfigVersion.cmake"
        COMPATIBILITY SameMajorVersion
        ARCH_INDEPENDENT
)

# Allow package maintainers to freely override the path for the configs
set(
        ${PROJECT_NAME}_INSTALL_CMAKEDIR "${CMAKE_INSTALL_DATADIR}/${package}"
        CACHE PATH "CMake package config location relative to the install prefix"
)
mark_as_advanced(${PROJECT_NAME}_INSTALL_CMAKEDIR)

set(project_config_in "${CMAKE_CURRENT_LIST_DIR}/${PROJECT_NAME}Config.cmake.in")

configure_package_config_file(
        "${project_config_in}"
        "${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
        INSTALL_DESTINATION "${${PROJECT_NAME}_INSTALL_CMAKEDIR}"
)
install(
        FILES "${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
        DESTINATION "${${PROJECT_NAME}_INSTALL_CMAKEDIR}"
        COMPONENT ${PROJECT_NAME}_Development
)

install(
        FILES "${PROJECT_BINARY_DIR}/${package}ConfigVersion.cmake"
        DESTINATION "${${PROJECT_NAME}_INSTALL_CMAKEDIR}"
        COMPONENT ${PROJECT_NAME}_Development
)

install(
        EXPORT ${PROJECT_NAME}Targets
        NAMESPACE ${PROJECT_NAME}::
        DESTINATION "${${PROJECT_NAME}_INSTALL_CMAKEDIR}"
        COMPONENT ${PROJECT_NAME}_Development
)

if (PROJECT_IS_TOP_LEVEL)
    include(CPack)
endif ()
