# MySchedule 日程管理器

`MySchedule` 是一个面向 Linux、使用 C++17 和 CMake 渐进开发的本地日程管理软件.当前分支已完成核心账户管理、任务管理以及后台提醒模块，支持任务到期检测和音乐提醒播放。

## 已实现

- 建立 `src/core/` 与 `src/cli/` 目录结构；
- 提供任务 ID、名称、开始时间、优先级、分类、提醒时间和提醒状态；
- 提供 `myschedule --help`、`myschedule help` 与 `myschedule run`；
- 对无参数和未知命令返回错误；
- 使用 CMake 构建，并通过 CTest 验证基础功能；
- 忽略构建产物、运行数据、日志和本地配置。
- 支持用户名注册、正确密码登录和错误密码拒绝；
- 使用每用户独立 salt 与 salted FNV-1a 保存密码，不保存明文；
- 将账户保存到 `data/users.txt`，并为不同用户生成独立任务文件路径。
- 严格校验日期时间、优先级、分类和提醒时间；
- 支持 Core 层任务添加、删除、全字段修改、日查询、月查询和全部查询；
- 拒绝相同开始时间，并按真实开始时间排序查询结果；
- 为优先级、分类和提醒时间提供默认值；
- 使用每用户独立任务文件保存当前任务列表。
- 支持直接命令形式的 `addtask`、`showtask`、`deltask` 和 `updatetask`；
- 支持交互式 `run` 登录、首次注册、Shell 风格命令循环和引号任务名；
- 使用 UTF-8 显示宽度处理中文表格和长任务名截断；
- 登录后启动后台提醒线程，主线程等待输入时仍可提醒，并在退出时安全停止和 `join`；
- 提醒触发后会更新并保存 `reminded` 状态，避免重复提醒。

Qt5 图形界面尚未实现。当前任务文件仍使用直接覆写，且 `next_id` 尚未独立持久化；原子替换、保存失败回滚、损坏文件全面拒绝和删除后永不复用 ID 将在 Step 5 完成。后续安排见 [开发路线图](docs/roadmap.md)。

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
        ├── reminder.cpp
        ├── reminder.h
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
./build/myschedule run
./build/myschedule user1 123456 addtask --name "Write homework" --start "2026-07-10 14:30"
./build/myschedule user1 123456 showtask --all
```

## 测试

```bash
ctest --test-dir build --output-on-failure
bash -n tests/test_cli.sh
bash tests/test_cli.sh
```

当前 CTest 覆盖两个帮助入口，账户注册、登录、salt 隔离和本地文件验证，任务字段校验、默认值、增删改查、筛选、排序和基础持久化，以及 Step 4 CLI、交互式 run、UTF-8 表格、引号解析和后台提醒；测试矩阵会随功能逐步扩展。
Reminder 模块已完成编译验证，并已单独验证 alarm.mp3 音频播放功能。完整提醒流程将在 CLI 集成完成后进行进一步测试。
