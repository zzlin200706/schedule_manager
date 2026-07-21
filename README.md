# MySchedule 日程管理器

`MySchedule` 是一个面向 Linux、使用 C++17 和 CMake 渐进开发的本地日程管理软件。当前仓库处于 Day 1，只提供可构建的项目骨架和最小命令行帮助。

## Day 1 已实现

- 建立 `src/core/` 与 `src/cli/` 目录结构；
- 提供包含 `id` 和 `name` 的最小 `Task` 数据模型；
- 提供 `myschedule --help` 与 `myschedule help`；
- 对无参数和未知命令返回错误；
- 使用 CMake 构建，并通过 CTest 验证两种帮助入口；
- 忽略构建产物、运行数据、日志和本地配置。

账户、任务增删改查、文件持久化、后台提醒和 Qt5 图形界面均尚未实现。后续安排见 [开发路线图](docs/roadmap.md)。

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

当前 CTest 只覆盖 Day 1 的两个帮助入口；测试矩阵会随功能逐日扩展。
