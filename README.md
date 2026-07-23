# MySchedule 日程管理器

`MySchedule` 是一个面向 Linux、使用 C++17 和 CMake 渐进开发的本地日程管理软件。当前分支处于 Step 5，已提供账户、任务管理、完整 CLI、后台提醒和强化的本地持久化能力。

## 已实现

- 建立 `src/core/` 与 `src/cli/` 目录结构；
- 提供任务 ID、名称、开始时间、优先级、分类、提醒时间和提醒状态；
- 提供 `myschedule --help`、`myschedule help` 与 `myschedule run`；
- 对无参数和未知命令返回错误；
- 使用 CMake 构建应用和可直接运行的 C++ 测试程序；
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

Qt5 图形界面尚未实现，将在 Step 6 完成。后续安排见 [开发路线图](docs/roadmap.md)。

## 项目结构

```text
schedule_manager_new/
├── CMakeLists.txt
├── README.md
├── data/
│   └── .gitkeep
├── docs/
│   └── roadmap.md
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

## 测试

```bash
./build/TaskManagerUpdateTest
bash -n tests/test_cli.sh
bash tests/test_cli.sh
```

当前回归覆盖帮助入口、账户注册与登录、salt 隔离、任务增删改查、筛选排序、完整 CLI、交互式 `run`、UTF-8 表格、后台提醒、ID 持久化、原子保存、失败回滚和损坏文件拒绝。C++ 测试程序由 CMake 构建后直接运行。
