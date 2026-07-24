# MySchedule 日程管理器

`MySchedule` 是一个面向 Linux、使用 C++17、CMake 和 Qt5 渐进开发的本地日程管理软件。当前分支处于 Step 6，已提供账户、任务管理、完整 CLI、后台提醒、强化的本地持久化能力和 Qt5 图形界面。

## 已实现

- 建立 `src/core/` 与 `src/cli/` 目录结构；
- 提供任务 ID、名称、开始时间、优先级、分类、提醒时间和提醒状态；
- 提供 `myschedule --help`、`myschedule help` 与 `myschedule run`；
- 对无参数和未知命令返回错误；
- 使用 CMake 构建 CLI/Core 和 Qt5 图形界面；
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
- 在任务文件中持久化 `next_id`，删除任意任务或重启程序后均不会复用历史 ID；
- 账户和任务数据先完整写入临时文件，再通过 `rename` 原子替换正式文件；
- 添加、修改、删除及提醒状态保存失败时回滚内存状态，且不改变原文件；
- 严格拒绝损坏的账户、任务、任务 ID 和 `next_id` 数据，不静默覆盖损坏文件；
- 无参数或 `run` 参数非法时在账户存储初始化前退出，不创建数据文件。
- 提供 Qt5 登录、注册、任务列表、任务添加、删除、全字段修改、日期筛选、月份切换、空状态和提醒弹窗；
- Qt 与 CLI 共用 Core，使用相同账户文件、任务文件、ID、时间、排序和唯一性规则；
- 关闭 Qt 主窗口或退出登录时停止界面提醒定时器。

音频提醒将在 Step 7 合并成员 B 的 CLI 音频 PR，并在 Step 8 下沉到 Core；语音识别将在 Step 8 对齐参考项目。后续安排见 [开发路线图](docs/roadmap.md)。

## 项目结构

```text
schedule_manager_new/
├── CMakeLists.txt
├── README.md
├── data/
│   └── .gitkeep
├── docs/
│   └── roadmap.md
├── Frontend/
│   ├── CMakeLists.txt
│   ├── include/
│   ├── resources/
│   ├── src/
│   └── style/
└── src/
    ├── cli/
    │   ├── app.cpp
    │   ├── app.h
    │   ├── cli.cpp
    │   ├── cli.h
    │   └── main.cpp
    └── core/
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
        ├── user.cpp
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

Qt5 图形界面：

```bash
cmake -S Frontend -B build-qt
cmake --build build-qt -j
./ScheduleManagerGUI
```

本仓库不保留 CTest、Shell 测试、Python 单元测试或其他程序测试内容。
