# MySchedule 开发路线图

项目按九个阶段开发。除成员 B 已有音频提醒 PR 外，每个阶段从最新 `main` 创建独立功能分支，在该阶段保持可构建，并只描述已经完成的应用功能。

| 阶段 | 分支 | 状态 | 目标 |
|---|---|---|---|
| Step 1 | `step1/project-bootstrap` | 已完成 | 项目骨架、最小 `Task` 模型、CLI 入口和帮助 |
| Step 2 | `step2/account-storage` | 已完成 | 用户注册登录、salted FNV-1a 密码 Hash 和基础文件路径 |
| Step 3 | `step3/task-management` | 已完成 | 完整任务模型、严格时间解析、增删改查和查询排序 |
| Step 4 | `step4/cli-reminder` | 已完成 | 完整 CLI、交互式 `run` 循环和后台提醒线程 |
| Step 5 | `step5/persistence-hardening` | 已完成 | ID 持久化、原子替换、保存失败回滚和损坏文件拒绝 |
| Step 6 | `step6/qt-frontend` | 已实现，待提交 | 复用 Core 的 Linux Qt5 图形界面 |
| Step 7 | 成员 B 真实 PR 分支 | 计划中 | 合并成员 B 的 CLI 音频提醒 PR |
| Step 8 | `step8/voice-recognition` | 计划中 | Core 音频提醒重构、Qt 音频复用和语音识别 |
| Step 9 | `step9/docs-release` | 计划中 | 文档、清理、应用一致性检查和发布候选 |

## 当前阶段边界

当前分支已实现 Step 6 的 Qt5 图形界面。在 Step 5 的持久化强化基础上，新增登录、注册、主窗口、任务列表、添加、删除、全字段修改、日期筛选、月份切换、空状态和提醒弹窗。

Qt 通过 `CoreAdapter` 复用现有 Core，和 CLI 使用相同账户文件、任务文件、ID、时间、排序和唯一性规则。当前阶段只构建普通 Qt 日程界面，不加入 Qt 测试工程、音频提醒、语音录音按钮、Whisper STT、DeepSeek 字段解析或本地 FastAPI 语音服务。

后续 Step 7 会合并成员 B 已有 CLI 音频提醒 PR；Step 8 会将音频提醒下沉到 Core，并按 `schedule_manager_clean` 加入 Qt 语音识别。项目不计划实现数据库、网络同步、Web 前端、Qt6 迁移或生产级密码系统。
