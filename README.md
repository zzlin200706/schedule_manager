# MySchedule 日程管理器

`MySchedule` 是一个面向 Linux 的本地日程管理项目，使用 C++17 和 CMake，提供命令行程序与 Qt5 Widgets 图形界面。账号和任务保存在本机文本文件中；可选语音功能会启动绑定本机的 Python 服务，并将用户确认后的识别文本发送给 DeepSeek API 解析任务字段。

```text
CLI    -> core -> data/
Qt GUI -> CoreAdapter -> core -> data/
```

顶层 CMake 工程构建 CLI 和 Core；`Frontend/` 是独立 Qt 工程。GUI 通过 `CoreAdapter` 复用 `src/core/` 中的账号、任务、存储、提醒和音频逻辑，因此 CLI 与 Qt 使用同一套数据规则。架构和语音流程见 [docs/design.md](docs/design.md)，开发阶段记录见 [docs/roadmap.md](docs/roadmap.md)，课程心得见 [docs/reflection.md](docs/reflection.md)，提交资料自查见 [docs/submission_checklist.md](docs/submission_checklist.md)。

## 已实现功能

CLI：

- `run` 交互模式：注册或登录、添加任务、修改任务、查询任务、删除任务和后台提醒；
- 直接命令模式：登录已有账号后添加、修改、查询或删除任务；
- 按全部、日期或月份查询，并按真实开始时间排序显示；
- 添加、修改、删除以及提醒状态变化后写入本地文件；
- 提醒触发时输出文字，并通过 Core 音频模块异步调用 `mpg123` 播放 `resources/alarm.mp3`。

Qt GUI：

- 注册、登录、退出登录并加载当前用户的任务；
- 添加、修改和删除任务；
- 显示分类、优先级、开始时间和提醒时间；
- 使用日历按日期筛选任务；
- 主窗口运行时每 10 秒检查一次提醒，通过非模态弹窗提示、异步播放提醒音频，并持久化 `reminded` 状态；
- 默认启用语音助手并显示录音入口，支持录音、识别文本确认、DeepSeek 字段解析和任务表单回填。

语音功能默认通过构建选项启用。启用后 Qt 使用 `arecord` 录制本地 WAV，通过 `voice_service/` 中的 FastAPI 服务调用 `faster-whisper` 完成语音识别，再由 DeepSeek 提取任务名称、开始时间、优先级、分类和提醒时间。解析结果经 C++ 侧校验后填入现有任务表单，最终由用户点击原有添加按钮保存。

CLI 和 Qt 共用 `src/core/audio_alert.*` 播放提醒音频，固定使用仓库根目录的 `resources/alarm.mp3`。播放在后台线程执行，带有单次播放保护；一轮扫描发现多个到期任务时统一播放一次。文字/弹窗提醒和 `reminded` 状态持久化独立于音频播放结果。

## 项目结构

以下列出 Git 跟踪的主要结构，构建产物和本地运行数据由 `.gitignore` 管理：

```text
schedule_manager/
├── CMakeLists.txt
├── README.md
├── .gitignore
├── data/
│   └── .gitkeep
├── docs/
│   ├── design.md
│   ├── reflection.md
│   ├── submission_checklist.md
│   └── roadmap.md
├── resources/
│   └── alarm.mp3
├── src/
│   ├── cli/
│   ├── core/
│   └── voice/
├── tests/
│   └── cli_crud_test.sh
├── Frontend/
│   ├── CMakeLists.txt
│   ├── include/
│   ├── resources/
│   ├── src/
│   └── style/
└── voice_service/
    ├── app.py
    ├── config.py
    ├── deepseek_task_parser.py
    ├── requirements.txt
    ├── schemas.py
    ├── stt_service.py
    └── README.md
```

- `src/core/`：账号、任务、存储、时间解析、提醒线程和 CLI/Qt 共用的音频播放；
- `src/cli/`：CLI 入口、交互流程和参数解析；
- `src/voice/`：Qt 语音解析结果的数据结构、错误类型和任务字段校验；
- `tests/`：命令行增删查改测试脚本；
- `Frontend/`：Qt GUI、`CoreAdapter`、样式和资源；
- `voice_service/`：本地 FastAPI 服务边界，承载 Whisper STT 和 DeepSeek 任务字段解析；
- `data/`：默认运行时数据目录；Git 只跟踪 `.gitkeep`。

## 环境要求

- Linux；当前代码使用 `/proc/self/exe`、`unistd.h`、`termios.h` 和 `arecord`；
- 支持 C++17 的编译器；
- CMake 3.16 或更高版本；
- 构建 GUI 时需要 Qt5 Core 和 Qt5 Widgets；
- 启用语音助手时需要 Qt5 Network；
- 使用语音功能需要 `arecord`、Python 3、`voice_service/requirements.txt` 中的依赖以及 DeepSeek API Key；
- 使用音乐提醒需要 `mpg123`；提醒音频固定为仓库根目录的 `resources/alarm.mp3`。

## 构建

以下命令均从仓库根目录执行。

### CLI

```bash
cmake -S . -B build
cmake --build build -j
```

生成的程序为 `build/myschedule`。

### Qt GUI

```bash
cmake -S Frontend -B Frontend/build
cmake --build Frontend/build -j
```

Qt 工程将程序输出为 `Frontend/ScheduleManagerGUI`。

### 关闭语音助手的 Qt GUI

Qt GUI 默认启用语音助手。没有 Qt Network 或不需要语音功能时：

```bash
cmake -S Frontend -B Frontend/build -DENABLE_VOICE_ASSISTANT=OFF
cmake --build Frontend/build -j
```

本地 Voice AI Service 默认绑定本机：

```bash
cd voice_service
python3 -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
cp .env.example .env
uvicorn app:app --host 127.0.0.1 --port 8765
```

Qt 语音按钮在首次使用时会检查 `127.0.0.1:8765/health`。服务运行中时直接复用；需要启动时，会尝试从 `voice_service/.venv/bin/python` 拉起服务。

## CLI 使用

查看帮助：

```bash
./build/myschedule --help
./build/myschedule help
```

交互模式：

```bash
./build/myschedule run
```

`run` 会提示输入用户名和密码。新用户名可以在提示后创建账号；直接命令模式用于登录已有账号。下面的示例假定已经创建账号 `user1`，密码为 `123456`。

添加任务：

```bash
./build/myschedule user1 123456 addtask \
  --name "写程序作业" \
  --start "2099-07-10 14:30" \
  --priority high \
  --category study \
  --remind "2099-07-10 14:20"
```

修改任务：

```bash
./build/myschedule user1 123456 updatetask --id 1 \
  --name "写程序作业最终版" \
  --start "2099-07-10 15:00" \
  --priority medium \
  --category study \
  --remind "2099-07-10 14:45"
```

查询任务：

```bash
./build/myschedule user1 123456 showtask --all
./build/myschedule user1 123456 showtask --day 2099-07-10
./build/myschedule user1 123456 showtask --month 2099-07
```

删除任务：

```bash
./build/myschedule user1 123456 deltask --id 1
./build/myschedule user1 123456 deltask 1
```

CLI 的 `run` 交互模式会启动提醒线程，直接命令聚焦单次增删改查。提醒线程约每 10 秒检查一次；程序运行期间，到期任务会输出文本提示、异步播放一次提醒音频，并将 `reminded` 写入任务文件，避免下次加载后重复提醒。

## 测试

构建 CLI 后运行测试脚本：

```bash
bash tests/cli_crud_test.sh
```

脚本默认调用 `build/myschedule`，也可以显式传入可执行程序路径：

```bash
bash tests/cli_crud_test.sh ./build/myschedule
```

测试会使用临时 `MYSCHEDULE_DATA_DIR`，先创建测试账号，再通过直接命令依次验证添加任务、全部查询、按日查询、按月查询、修改任务和删除任务，不会污染仓库的 `data/`。

## Qt GUI 使用

在有图形会话的 Linux 环境中，从仓库根目录运行：

```bash
./Frontend/ScheduleManagerGUI
```

GUI 的注册、登录、任务读取、添加、修改、删除和提醒检查均通过 `CoreAdapter` 调用 `src/core/`。有到期提醒时，GUI 会显示非模态弹窗并通过 Core 异步播放提醒音频。GUI 提醒随主窗口生命周期运行。

默认启用语音助手后，任务输入区会出现录音按钮。录音完成后，GUI 会把 WAV 上传到本地服务，显示识别文本供用户确认，再请求 DeepSeek 解析任务字段。合法结果会填入表单；保存仍由用户点击添加按钮完成。

## 数据文件

默认数据文件为：

```text
data/users.txt
data/tasks_<username>.txt
```

- `users.txt` 的每行格式为 `username:salt:password_hash`；
- 每个账号对应一个任务文件；
- 任务文件的表头和 ID 元数据为：

```text
id|name|start_time|priority|category|remind_time|reminded
next_id|2
```

`next_id` 是该用户下一个可分配的 ID，用于保证 ID 单调递增。旧版任务文件仍可加载，下次保存时会写入该元数据。

这些运行时文件由 `.gitignore` 管理，保留在本地运行环境中。若把可执行文件复制到仓库外，建议显式设置 `MYSCHEDULE_DATA_DIR`：

```bash
MYSCHEDULE_DATA_DIR=/tmp/myschedule-data ./build/myschedule run
```

## 输入规则

- 用户名支持 ASCII 字母、数字、下划线和连字符；密码要求非空；
- 优先级支持 `high`、`medium`、`low`，分类支持 `study`、`entertainment`、`life`；
- 时间格式为 `YYYY-MM-DD HH:MM`，提醒时间应早于或等于开始时间；
- 同一账号下任务开始时间保持唯一；任务名使用普通文本，避开 `|`、回车和换行；
- 任务 ID 由任务文件中持久化的 `next_id` 递增分配；每个账号有自己的 ID 序列；
- CLI 交互模式支持单引号、双引号和反斜杠转义；含空格的名称和时间必须用引号包围；
- CLI 和 GUI 在进程运行期间检查提醒，通过应用内文字或弹窗提示并播放音频提醒；
- CLI 和 GUI 音乐提醒使用外部 `mpg123`，并从仓库根目录定位固定的 `resources/alarm.mp3`；文字/弹窗提示与音频播放结果相互独立；
- 数据保存在本地文本文件中；
- 直接命令会把密码放在命令行参数中，可能出现在 shell 历史和进程列表中；需要避免这一点时使用 `run` 交互模式或 GUI；
- `TaskManager` 的互斥锁保护同一进程内的任务访问；同一账号的数据修改建议集中在一个客户端中完成；
- 单次保存先完整写入同目录临时文件，检查写入和关闭状态后再用 `rename` 替换旧文件；
- 语音模块通过 Qt 录音入口调用 Whisper 和 DeepSeek，自动填充任务名称、开始时间、优先级、分类和提醒提前量，任务保存由用户确认完成；
- 密码使用每用户独立 salt 和 FNV-1a 64 位哈希保存，作为课程项目中的本地认证实现示例。
