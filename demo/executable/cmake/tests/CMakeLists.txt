set(KCONFIG_DEMO_CASES_SCRIPT
    ${CMAKE_CURRENT_LIST_DIR}/demo_config_cases.sh
)

add_test(
    NAME demo_config_baseline_success
    COMMAND ${KCONFIG_DEMO_CASES_SCRIPT}
            $<TARGET_FILE:kconfig_demo_executable>
            baseline_success
)

add_test(
    NAME demo_config_user_override_success
    COMMAND ${KCONFIG_DEMO_CASES_SCRIPT}
            $<TARGET_FILE:kconfig_demo_executable>
            config_user_override_success
)

add_test(
    NAME demo_config_cli_override_success
    COMMAND ${KCONFIG_DEMO_CASES_SCRIPT}
            $<TARGET_FILE:kconfig_demo_executable>
            cli_config_override_success
)
