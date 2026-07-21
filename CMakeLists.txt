cmake_minimum_required(VERSION 3.16)
project(myschedule LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(schedule_core
    src/core/task.cpp
)

target_include_directories(schedule_core PUBLIC src/core)
target_compile_options(schedule_core PRIVATE -Wall -Wextra -Wpedantic)

add_executable(myschedule
    src/cli/main.cpp
    src/cli/app.cpp
    src/cli/cli.cpp
)

target_include_directories(myschedule PRIVATE src/cli)
target_link_libraries(myschedule PRIVATE schedule_core)
target_compile_options(myschedule PRIVATE -Wall -Wextra -Wpedantic)

include(CTest)
if(BUILD_TESTING)
    add_test(NAME cli_help_long COMMAND myschedule --help)
    set_tests_properties(cli_help_long PROPERTIES PASS_REGULAR_EXPRESSION "Usage:")

    add_test(NAME cli_help_command COMMAND myschedule help)
    set_tests_properties(cli_help_command PROPERTIES PASS_REGULAR_EXPRESSION "Commands:")
endif()
