cmake_minimum_required(VERSION 3.20)

project(kconfig_demo_compile LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(KConfigSDK CONFIG REQUIRED)

add_executable(demo_compile_client
    src/client/main.cpp
)
target_link_libraries(demo_compile_client PRIVATE kconfig::sdk)
set_target_properties(demo_compile_client PROPERTIES OUTPUT_NAME client)

add_executable(demo_compile_server
    src/server/main.cpp
)
target_link_libraries(demo_compile_server PRIVATE kconfig::sdk)
set_target_properties(demo_compile_server PROPERTIES OUTPUT_NAME server)
