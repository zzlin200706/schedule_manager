# MySchedule 日程管理器

MySchedule 是一个面向 Linux、使用 C++17 和 CMake 渐进开发的本地日程管理软件。当前分支已完成核心账户管理、任务管理以及后台提醒模块，支持任务到期检测和音乐提醒播放。

## 已实现

- 建立 `src/core/` 与 `src/cli/` 目录结构；
- 提供任务 ID、名称、开始时间、优先级、分类、提醒时间和提醒状态；
- 提供 `myschedule --help` 与 `myschedule help`；
- 对无参数和未知命令返回错误；
- 使用 CMake 构建，并通过 CTest 验证基础功能；
- 忽略构建产物、运行数据、日志和本地配置。

### 用户管理

- 支持用户名注册；
- 支持正确密码登录和错误密码拒绝；
- 使用每用户独立 salt 与 salted FNV-1a 保存密码，不保存明文密码；
- 将账户保存到 `data/users.txt`；
- 为不同用户生成独立任务文件路径。

### 任务管理

- 严格校验日期时间、优先级、分类和提醒时间；
- 支持 Core 层任务添加、删除、全字段修改；
- 支持按日查询、按月查询和全部查询；
- 拒绝相同开始时间的任务冲突；
- 按真实开始时间排序查询结果；
- 为优先级、分类和提醒时间提供默认值；
- 使用每用户独立任务文件保存任务列表。

### 后台提醒与音乐提醒

- 新增 `Reminder` 模块，实现后台提醒检测；
- 支持扫描即将到期任务；
- 添加 `resources/alarm.mp3` 音频资源；
- 提醒触发时调用 `mpg123` 播放提示音；
- 保持原有任务管理结构不变。

## 项目结构

```text
schedule_manager_new/
├── CMakeLists.txt
├── README.md
├── data/
│   └── .gitkeep
├── docs/
│   └── roadmap.md
├── resources/
│   └── alarm.mp3
└── src/
    ├── cli/
    │   ├── app.cpp
    │   ├── app.h
    │   ├── cli.cpp
    │   ├── cli.h
    │   └── main.cpp
    └── core/
        ├── reminder.cpp
        ├── reminder.h
        ├── storage.cpp
        ├── storage.h
        ├── task.cpp
        ├── task.h
        ├── task_manager.cpp
        ├── task_manager.h
        ├── time_utils.cpp
        ├── time_utils.h
        └── user.cpp
        └── user.h

 ```

## 构建与运行

以下命令均从仓库根目录执行：

```bash
cmake -S . -B build
cmake --build build -j
./build/myschedule --help
./build/myschedule help
```

## 测试

```bash
ctest --test-dir build --output-on-failure
```

当前 CTest 覆盖 CLI 帮助入口、账户注册登录、salt 隔离、本地文件验证，以及任务字段校验、默认值、增删改查、筛选、排序和基础持久化。
Reminder 模块已完成编译验证，并已单独验证 alarm.mp3 音频播放功能。完整提醒流程将在 CLI 集成完成后进行进一步测试。
