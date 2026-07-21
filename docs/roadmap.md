# MySchedule 开发路线图

项目按七个阶段开发。每个阶段从最新 `main` 创建独立功能分支，在该阶段保持可构建、可测试，并只描述已经完成的功能。

| 阶段 | 分支 | 状态 | 目标 |
|---|---|---|---|
| Day 1 | `day1/project-bootstrap` | 已完成 | 项目骨架、最小 `Task` 模型、CLI 入口和帮助 |
| Day 2 | `day2/account-storage` | 计划中 | 用户注册登录、salted FNV-1a 密码 Hash 和基础文件路径 |
| Day 3 | `day3/task-management` | 计划中 | 完整任务模型、严格时间解析、增删改查和查询排序 |
| Day 4 | `day4/cli-reminder` | 计划中 | 完整 CLI、交互式 `run` 循环和后台提醒线程 |
| Day 5 | `day5/persistence-hardening` | 计划中 | ID 持久化、原子替换、保存失败回滚和损坏文件拒绝 |
| Day 6 | `day6/qt-frontend` | 计划中 | 复用 Core 的 Linux Qt5 图形界面和 Qt 测试 |
| Day 7 | `day7/tests-docs-release` | 计划中 | 完整回归测试、文档、一致性检查和发布候选 |

## 当前阶段边界

Day 1 只实现项目骨架和帮助命令。当前 `Task` 仅包含 `id` 与 `name`；开始时间、优先级、分类和提醒字段将在 Day 3 加入。账户与基础存储从 Day 2 开始实现，任务命令和提醒服务分别在 Day 3、Day 4 进入项目。

Qt5 图形界面是 Day 6 的选做功能，在完成前不会出现在已实现功能列表中。项目不计划实现数据库、网络同步、云服务、Web 前端、音乐提醒、录音提醒或语音识别。
