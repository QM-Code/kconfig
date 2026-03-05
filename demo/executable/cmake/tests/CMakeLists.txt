set(KCONFIG_DEMO_CASES_SCRIPT
    ${CMAKE_CURRENT_LIST_DIR}/demo_config_cases.sh
)

add_test(
    NAME demo_config_baseline_success
    COMMAND ${KCONFIG_DEMO_CASES_SCRIPT}
            $<TARGET_FILE:kconfig_demo_executable>
            baseline_success
)
