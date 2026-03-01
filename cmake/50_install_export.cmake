include(CMakePackageConfigHelpers)

set(KCONFIG_INSTALL_CMAKEDIR "lib/cmake/KConfigSDK")

install(TARGETS kconfig_sdk
    EXPORT KConfigSDKTargets
    ARCHIVE DESTINATION lib COMPONENT KConfigSDK
    LIBRARY DESTINATION lib COMPONENT KConfigSDK
    RUNTIME DESTINATION bin COMPONENT KConfigSDK
    INCLUDES DESTINATION include
)

install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/
    DESTINATION include
    COMPONENT KConfigSDK
    FILES_MATCHING PATTERN "*.hpp"
)

install(EXPORT KConfigSDKTargets
    FILE KConfigSDKTargets.cmake
    NAMESPACE kconfig::
    DESTINATION ${KCONFIG_INSTALL_CMAKEDIR}
    COMPONENT KConfigSDK
)

configure_package_config_file(
    ${PROJECT_SOURCE_DIR}/cmake/KConfigSDKConfig.cmake.in
    ${PROJECT_BINARY_DIR}/KConfigSDKConfig.cmake
    INSTALL_DESTINATION ${KCONFIG_INSTALL_CMAKEDIR}
)

write_basic_package_version_file(
    ${PROJECT_BINARY_DIR}/KConfigSDKConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    ${PROJECT_BINARY_DIR}/KConfigSDKConfig.cmake
    ${PROJECT_BINARY_DIR}/KConfigSDKConfigVersion.cmake
    DESTINATION ${KCONFIG_INSTALL_CMAKEDIR}
    COMPONENT KConfigSDK
)
