# MySchedule 开发路线图

项目按七个阶段开发。每个阶段从最新 `main` 创建独立功能分支，在该阶段保持可构建、可测试，并只描述已经完成的功能。

| 阶段 | 分支 | 状态 | 目标 |
|---|---|---|---|
| Step 1 | `step1/project-bootstrap` | 已完成 | 项目骨架、最小 `Task` 模型、CLI 入口和帮助 |
| Step 2 | `step2/account-storage` | 已完成 | 用户注册登录、salted FNV-1a 密码 Hash 和基础文件路径 |
| Step 3 | `step3/task-management` | 已完成 | 完整任务模型、严格时间解析、增删改查和查询排序 |
| Step 4 | `step4/cli-reminder` | 已实现，待提交 | 完整 CLI、交互式 `run` 循环和后台提醒线程 |
| Step 5 | `step5/persistence-hardening` | 计划中 | ID 持久化、原子替换、保存失败回滚和损坏文件拒绝 |
| Step 6 | `step6/qt-frontend` | 计划中 | 复用 Core 的 Linux Qt5 图形界面和 Qt 测试 |
| Step 7 | `step7/tests-docs-release` | 计划中 | 完整回归测试、文档、一致性检查和发布候选 |

## 当前阶段边界

当前分支已实现 Step 4 的完整 CLI 参数、直接任务命令、交互式 `run` 登录和注册、Shell 风格命令解析、引号任务名、UTF-8 表格输出、长名称截断和后台提醒线程。提醒服务使用 Core 任务数据，主线程等待输入时仍会检查到期提醒，并在退出时安全停止线程。

任务文件的 `next_id` 元数据、删除后永不复用 ID、临时文件替换、保存失败回滚和更严格的损坏文件处理将在 Step 5 完成。当前阶段提醒触发后会保存 `reminded` 状态，但保存失败时的内存回滚也将在 Step 5 统一强化。当前阶段保留这些差异，以便每一步都对应清晰且可验证的新增功能。

Qt5 图形界面是 Step 6 的选做功能，在完成前不会出现在已实现功能列表中。项目不计划实现数据库、网络同步、云服务、Web 前端、音乐提醒、录音提醒或语音识别。
