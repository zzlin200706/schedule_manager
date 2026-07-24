cmake_minimum_required(VERSION 3.16)
project(ScheduleManagerGUI)

option(ENABLE_VOICE_ASSISTANT "Enable voice task assistant" OFF)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 查找Qt5库
find_package(Qt5 REQUIRED COMPONENTS Core Widgets)
find_package(Threads REQUIRED)
if(ENABLE_VOICE_ASSISTANT)
    find_package(Qt5 REQUIRED COMPONENTS Network)
endif()

# 启用 Qt 的 MOC 和资源编译
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# 设置包含目录
include_directories(include)

# GUI 与 core 之间的适配及共用 core 源文件
set(CORE_BRIDGE_SOURCES
    src/core_adapter.cpp
    src/task.cc
    ../src/core/user.cpp
    ../src/core/storage.cpp
    ../src/core/task.cpp
    ../src/core/task_manager.cpp
    ../src/core/time_utils.cpp
    ../src/core/audio_alert.cpp
)

# 源文件 (在src目录下)
set(SOURCES
    src/main.cpp
    src/mainwindow.cc
    src/login_window.cpp
    src/register_window.cpp
    ${CORE_BRIDGE_SOURCES}
    resources/styles.qrc
)

# 头文件 (在include目录下)
set(HEADERS
    include/mainwindow.h
    include/login_window.h
    include/register_window.h
    include/core_adapter.h
    include/task.h
)

if(ENABLE_VOICE_ASSISTANT)
    list(APPEND SOURCES
        ../src/voice/task_data_validator.cpp
        ../src/voice/voice_result.cpp
    )
    list(APPEND HEADERS
        ../src/voice/parsed_task_data.h
        ../src/voice/task_data_validator.h
        ../src/voice/voice_result.h
    )
endif()

# 创建可执行文件
add_executable(ScheduleManagerGUI ${SOURCES} ${HEADERS})

target_include_directories(ScheduleManagerGUI PRIVATE
    include
    ../src/core
)

if(ENABLE_VOICE_ASSISTANT)
    target_compile_definitions(ScheduleManagerGUI PRIVATE ENABLE_VOICE_ASSISTANT)
    target_include_directories(ScheduleManagerGUI PRIVATE ../src/voice)
    set(VOICE_QT_LIBRARIES Qt5::Network)
endif()

target_compile_options(ScheduleManagerGUI PRIVATE
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:-finput-charset=UTF-8>
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:-fexec-charset=UTF-8>
    $<$<CXX_COMPILER_ID:MSVC>:/utf-8>
)

# 仅对项目维护的源文件启用常规警告，避免影响 Qt 自动生成代码。
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set_source_files_properties(${SOURCES} PROPERTIES
        COMPILE_OPTIONS "-Wall;-Wextra;-Wpedantic"
    )
endif()

# 链接Qt5库
target_link_libraries(ScheduleManagerGUI
    Qt5::Core
    Qt5::Widgets
    ${VOICE_QT_LIBRARIES}
    Threads::Threads
)

# 设置可执行文件输出到根目录
set_target_properties(ScheduleManagerGUI PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}
)
