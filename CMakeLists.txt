cmake_minimum_required(VERSION 3.16)
project(myschedule LANGUAGES CXX)

# 文件职责：
# 顶层 CLI 构建脚本。这里构建 schedule_core 静态/对象库和 myschedule
# 命令行程序；Qt GUI 的构建脚本位于 Frontend/CMakeLists.txt。

option(ENABLE_VOICE_ASSISTANT "Enable voice assistant application code" ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Threads REQUIRED)

# 核心层库：用户、任务、存储、提醒和音频逻辑都放在这里，CLI 直接链接使用。
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

# CLI 可执行程序入口在 src/cli/main.cpp，命令分发在 src/cli/app.cpp。
add_executable(myschedule
    src/cli/main.cpp
    src/cli/app.cpp
    src/cli/cli.cpp
)

target_include_directories(myschedule PRIVATE src/cli)
target_link_libraries(myschedule PRIVATE schedule_core)
target_compile_options(myschedule PRIVATE -Wall -Wextra -Wpedantic)
