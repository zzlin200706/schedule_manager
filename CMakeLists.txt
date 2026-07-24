cmake_minimum_required(VERSION 3.16)
project(myschedule LANGUAGES CXX)

option(ENABLE_VOICE_ASSISTANT "Enable voice assistant application code" OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Threads REQUIRED)

add_library(schedule_core
    src/core/task.cpp
    src/core/task_manager.cpp
    src/core/time_utils.cpp
    src/core/storage.cpp
    src/core/user.cpp
    src/core/reminder.cpp
    src/core/audio_alert.cpp
)

target_include_directories(schedule_core PUBLIC src/core)
target_link_libraries(schedule_core PUBLIC Threads::Threads)
target_compile_options(schedule_core PRIVATE -Wall -Wextra -Wpedantic)

add_executable(myschedule
    src/cli/main.cpp
    src/cli/app.cpp
    src/cli/cli.cpp
)

target_include_directories(myschedule PRIVATE src/cli)
target_link_libraries(myschedule PRIVATE schedule_core)
target_compile_options(myschedule PRIVATE -Wall -Wextra -Wpedantic)
