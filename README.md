# MySchedule 日程管理器

`MySchedule` 是一个面向 Linux、使用 C++17 和 CMake 渐进开发的本地日程管理软件。当前分支处于 Step 2，提供项目骨架、最小命令行帮助和核心账户存储。

## 已实现

- 建立 `src/core/` 与 `src/cli/` 目录结构；
- 提供包含 `id` 和 `name` 的最小 `Task` 数据模型；
- 提供 `myschedule --help` 与 `myschedule help`；
- 对无参数和未知命令返回错误；
- 使用 CMake 构建，并通过 CTest 验证两种帮助入口；
- 忽略构建产物、运行数据、日志和本地配置。
- 支持用户名注册、正确密码登录和错误密码拒绝；
- 使用每用户独立 salt 与 salted FNV-1a 保存密码，不保存明文；
- 将账户保存到 `data/users.txt`，并为不同用户生成独立任务文件路径。

交互式登录、任务增删改查、任务持久化、后台提醒和 Qt5 图形界面均尚未实现。后续安排见 [开发路线图](docs/roadmap.md)。

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
        ├── task.cpp
        └── task.h
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

当前 CTest 覆盖两个帮助入口以及账户注册、登录、salt 隔离和本地文件验证；测试矩阵会随功能逐步扩展。
