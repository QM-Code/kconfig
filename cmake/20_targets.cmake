set(KCONFIG_SOURCES
    ${PROJECT_SOURCE_DIR}/src/helpers.cpp
    ${PROJECT_SOURCE_DIR}/src/store.cpp
    ${PROJECT_SOURCE_DIR}/src/trace.cpp
    ${PROJECT_SOURCE_DIR}/src/validation.cpp
    ${PROJECT_SOURCE_DIR}/src/data/directory_override.cpp
    ${PROJECT_SOURCE_DIR}/src/data/path_resolver.cpp
    ${PROJECT_SOURCE_DIR}/src/data/path_utils.cpp
    ${PROJECT_SOURCE_DIR}/src/data/root_policy.cpp
    ${PROJECT_SOURCE_DIR}/src/i18n.cpp
)

if(KCONFIG_BUILD_SHARED)
    set(_kconfig_library_type SHARED)
else()
    set(_kconfig_library_type STATIC)
endif()

add_library(kconfig_sdk ${_kconfig_library_type} ${KCONFIG_SOURCES})
add_library(kconfig::sdk ALIAS kconfig_sdk)

target_include_directories(kconfig_sdk
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(kconfig_sdk
    PUBLIC
        ktrace::sdk
        spdlog::spdlog
        nlohmann_json::nlohmann_json
)

target_compile_definitions(kconfig_sdk
    PRIVATE
        KTRACE_NAMESPACE=\"kconfig\"
)

set_target_properties(kconfig_sdk PROPERTIES
    OUTPUT_NAME kconfig
    EXPORT_NAME sdk
)
