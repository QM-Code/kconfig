cmake_minimum_required(VERSION 3.20)

project(kconfig_demo_compile LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(KConfigSDK CONFIG REQUIRED)

add_executable(kconfig_demo_compile
    src/main.cpp
)
target_link_libraries(kconfig_demo_compile PRIVATE kconfig::sdk)
set_target_properties(kconfig_demo_compile PROPERTIES OUTPUT_NAME test)
