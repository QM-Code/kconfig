cmake_minimum_required(VERSION 3.20)

project(kconfig_demo_compile LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(KConfigSDK CONFIG REQUIRED)
find_package(KTraceSDK CONFIG REQUIRED)

add_executable(kconfig_demo_compile
    src/main.cpp
)
target_link_libraries(kconfig_demo_compile PRIVATE kconfig::sdk)
target_link_libraries(kconfig_demo_compile PRIVATE ktrace::sdk)
target_compile_definitions(kconfig_demo_compile
    PRIVATE
        KTRACE_NAMESPACE=\"kconfig_demo\"
)

set_target_properties(kconfig_demo_compile PROPERTIES OUTPUT_NAME test)

include(CTest)
if(BUILD_TESTING AND EXISTS "${CMAKE_CURRENT_LIST_DIR}/cmake/tests/CMakeLists.txt")
    add_subdirectory(cmake/tests)
endif()
