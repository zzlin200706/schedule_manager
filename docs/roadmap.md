# MySchedule 开发路线图

| 阶段 | 分支 | 状态 | 完成内容 |
|---|---|---|---|
| Step 1 | `step1/project-bootstrap` | 已完成 | 建立 CMake 项目骨架、最小 `Task` 模型、CLI 入口和帮助输出 |
| Step 2 | `step2/account-storage` | 已完成 | 加入用户注册登录、每用户 salt、salted FNV-1a 密码 Hash 和本地账户文件 |
| Step 3 | `step3/task-management` | 已完成 | 完成任务模型、时间解析、默认优先级/分类、增删改查、日/月/全部查询和排序 |
| Step 4 | `step4/cli-reminder` | 已完成 | 完成直接命令、交互式 `run` 循环、Shell 风格解析、后台提醒线程和提醒状态保存 |
| Step 5 | `step5/persistence-hardening` | 已完成 | 加入 `next_id` 持久化、临时文件加 `rename` 替换、保存失败回滚和损坏文件拒绝 |
| Step 6 | `step6/qt-frontend` | 已完成 | 加入复用 Core 的 Linux Qt5 登录、注册、任务管理、日期筛选、空状态和提醒弹窗 |
| Step 7 | `step7/music-reminder` | 已完成 | 合并成员 B 的音频提醒工作，让提醒触发时播放 `resources/alarm.mp3` |
| Step 8 | `step8/voice-recognition` | 已完成 | 将音频提醒下沉到 Core，补齐 Qt 音频复用、语音录音、本地 FastAPI 服务、Whisper STT、DeepSeek 字段解析和表单回填 |
| Step 9 | `step9/docs-release` | 整理中 | 更新发布文档、清理本地忽略规则、比较 `schedule_manager_clean` 和当前仓库的应用差异 |
