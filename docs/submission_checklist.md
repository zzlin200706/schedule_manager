# MySchedule 提交资料自查

## 文件结构

- `build/myschedule`：Linux x86-64 CLI 可执行程序，已保留执行权限。
- `Frontend/ScheduleManagerGUI`：Linux x86-64 Qt GUI 可执行程序，已保留执行权限。
- `tests/cli_crud_test.sh`：CLI 增删查改测试脚本，可从仓库根目录直接运行。
- `docs/design.md`：设计文档源文件，PDF 版位于提交包根目录。
- `docs/reflection.md`：课程学习心得源文件，PDF 版位于提交包根目录。

## 运行检查

从仓库根目录执行：

```bash
chmod +x build/myschedule Frontend/ScheduleManagerGUI tests/cli_crud_test.sh
bash tests/cli_crud_test.sh
```

测试脚本默认使用 `build/myschedule`，并通过临时 `MYSCHEDULE_DATA_DIR` 创建隔离数据目录，不会污染仓库的 `data/`。

## 打包要求

- 不保留 Windows `.lnk` 快捷方式。
- 不额外复制根目录的 CLI/GUI CMakeLists 文件。
- 不把 `voice_service/.venv`、`Frontend/build` 等本地环境目录放入提交包。
- 从 Linux 环境使用 `zip -r` 重新打包，避免丢失可执行权限。
