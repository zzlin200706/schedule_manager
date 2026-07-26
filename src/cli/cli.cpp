#include "cli.h"

/*
 * 文件职责：
 * 实现 CLI 帮助信息、任务表格输出、密码无回显读取，以及 add/update/show/delete
 * 命令参数解析。解析完成后的业务动作由 app.cpp 分发到核心层。
 *
 * 主要功能定位：
 * - 功能：CLI 命令分发
 * - addtask/updatetask/showtask/deltask 参数转换
 */

#include "time_utils.h"

#include <algorithm>
#include <clocale>
#include <cwchar>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <termios.h>
#include <unistd.h>

namespace {
std::string yesNo(bool value) {
    return value ? "yes" : "no";
}

struct TableColumn {
    const char* title;
    int width;
};

constexpr TableColumn kTaskColumns[] = {
    {"ID", 4},
    {"Name", 28},
    {"Start Time", 16},
    {"Priority", 8},
    {"Category", 13},
    {"Remind Time", 16},
    {"Reminded", 8},
};

int utf8DisplayWidth(const std::string& value) {
    // 计算终端显示宽度，避免中文任务名在表格中错位。
    std::mbstate_t state {};
    int width = 0;
    const char* ptr = value.data();
    std::size_t remaining = value.size();

    while (remaining > 0) {
        wchar_t wideChar = 0;
        const std::size_t consumed = std::mbrtowc(&wideChar, ptr, remaining, &state);
        if (consumed == static_cast<std::size_t>(-1) ||
            consumed == static_cast<std::size_t>(-2)) {
            ++ptr;
            --remaining;
            state = std::mbstate_t {};
            ++width;
            continue;
        }
        if (consumed == 0) {
            break;
        }

        const int charWidth = wcwidth(wideChar);
        width += std::max(charWidth, 1);
        ptr += consumed;
        remaining -= consumed;
    }
    return width;
}

std::string truncateUtf8ToWidth(const std::string& value, int maxWidth) {
    // 按显示宽度截断，而不是按字节截断，避免截断到 UTF-8 字符中间。
    constexpr const char* ellipsis = "...";
    constexpr int ellipsisWidth = 3;

    if (utf8DisplayWidth(value) <= maxWidth) {
        return value;
    }
    if (maxWidth <= ellipsisWidth) {
        return std::string(static_cast<std::size_t>(std::max(maxWidth, 0)), '.');
    }

    const int contentLimit = maxWidth - ellipsisWidth;
    std::string result;
    std::mbstate_t state {};
    int width = 0;
    const char* ptr = value.data();
    std::size_t remaining = value.size();

    while (remaining > 0) {
        wchar_t wideChar = 0;
        const std::size_t consumed = std::mbrtowc(&wideChar, ptr, remaining, &state);
        if (consumed == static_cast<std::size_t>(-1) ||
            consumed == static_cast<std::size_t>(-2)) {
            if (width + 1 > contentLimit) {
                break;
            }
            result.push_back(*ptr);
            ++ptr;
            --remaining;
            state = std::mbstate_t {};
            ++width;
            continue;
        }
        if (consumed == 0) {
            break;
        }

        const int charWidth = std::max(wcwidth(wideChar), 1);
        if (width + charWidth > contentLimit) {
            break;
        }
        result.append(ptr, consumed);
        width += charWidth;
        ptr += consumed;
        remaining -= consumed;
    }

    result += ellipsis;
    return result;
}

std::string padUtf8ToWidth(const std::string& value, int width) {
    std::string truncated = truncateUtf8ToWidth(value, width);
    const int padding = width - utf8DisplayWidth(truncated);
    if (padding > 0) {
        truncated.append(static_cast<std::size_t>(padding), ' ');
    }
    return truncated;
}

void printCell(std::ostream& output, const std::string& value, int width) {
    output << padUtf8ToWidth(value, width) << " | ";
}

class TerminalEchoGuard {
public:
    TerminalEchoGuard() = default;

    bool disable() {
        // 用 RAII 管理终端回显，readPassword 返回后析构函数会恢复原设置。
        if (!isatty(STDIN_FILENO)) {
            return false;
        }
        if (tcgetattr(STDIN_FILENO, &original_) != 0) {
            return false;
        }

        termios updated = original_;
        updated.c_lflag &= ~ECHO;
        if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &updated) != 0) {
            return false;
        }

        active_ = true;
        return true;
    }

    ~TerminalEchoGuard() {
        if (active_) {
            tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_);
        }
    }

private:
    termios original_ {};
    bool active_ = false;
};

bool parsePositiveTaskId(const std::string& value, int& taskId) {
    if (value.empty()) {
        return false;
    }

    for (const unsigned char ch : value) {
        if (ch < '0' || ch > '9') {
            return false;
        }
    }

    try {
        std::size_t consumed = 0;
        const int parsed = std::stoi(value, &consumed, 10);
        if (consumed != value.size() || parsed <= 0) {
            return false;
        }
        taskId = parsed;
        return true;
    } catch (const std::invalid_argument&) {
        return false;
    } catch (const std::out_of_range&) {
        return false;
    }
}
}

void CLI::printHelp(std::ostream& output) {
    output
        << "MySchedule - A command line schedule manager with accounts, task storage, and background reminders.\n\n"
        << "Usage:\n"
        << "  myschedule --help\n"
        << "  myschedule help\n"
        << "  myschedule run\n"
        << "  myschedule <username> <password> addtask --name <name> --start <time> [--priority high|medium|low] [--category study|entertainment|life] [--remind <time>]\n"
        << "  myschedule <username> <password> updatetask --id <id> [--name <name>] [--start <time>] [--priority high|medium|low] [--category study|entertainment|life] [--remind <time>]\n"
        << "  myschedule <username> <password> showtask --all\n"
        << "  myschedule <username> <password> showtask --day YYYY-MM-DD\n"
        << "  myschedule <username> <password> showtask --month YYYY-MM\n"
        << "  myschedule <username> <password> deltask --id <id>\n"
        << "  myschedule <username> <password> deltask <id>\n\n"
        << "Accounts:\n"
        << "  Use `myschedule run` for interactive login. If the username does not exist, run mode can create it.\n"
        << "  Direct commands use: myschedule <username> <password> <command> ...\n"
        << "  Passwords are stored as salted hashes, not plaintext. Command-line passwords may still be visible in shell history or process listings.\n\n"
        << "Commands:\n"
        << "  run         Load the user's tasks, enter an interactive shell, and check reminders in a background thread.\n"
        << "  addtask     Add a task and save immediately.\n"
        << "  updatetask  Update an existing task by its real task ID and save immediately.\n"
        << "  showtask    Show all tasks, tasks on one day, or tasks in one month.\n"
        << "  deltask     Delete a task by ID and save immediately.\n"
        << "  help        Show this help text.\n\n"
        << "Formats and defaults:\n"
        << "  Date/time:  YYYY-MM-DD HH:MM for --start and --remind.\n"
        << "  Day query:  YYYY-MM-DD for showtask --day.\n"
        << "  Month query: YYYY-MM for showtask --month.\n"
        << "  priority defaults to medium; allowed values: high | medium | low.\n"
        << "  category defaults to life; allowed values: study | entertainment | life.\n"
        << "  remind defaults to start time when omitted; reminded defaults to false.\n"
        << "  Task ID must be a complete decimal integer greater than 0.\n\n"
        << "Examples:\n"
        << "  myschedule run\n"
        << "  myschedule user1 123456 addtask --name \"Write homework\" --start \"2026-07-10 14:30\" --priority high --category study --remind \"2026-07-10 14:20\"\n"
        << "  myschedule user1 123456 addtask --name \"Read book\" --start \"2026-07-11 09:00\"\n"
        << "  myschedule user1 123456 updatetask --id 1 --name \"Write final homework\" --start \"2026-07-10 15:00\" --priority medium --category study --remind \"2026-07-10 14:45\"\n"
        << "  myschedule user1 123456 showtask --all\n"
        << "  myschedule user1 123456 showtask --day 2026-07-10\n"
        << "  myschedule user1 123456 showtask --month 2026-07\n"
        << "  myschedule user1 123456 deltask --id 1\n";
}

void CLI::printInteractiveHelp(std::ostream& output) {
    output
        << "Interactive commands:\n"
        << "  help\n"
        << "  addtask\n"
        << "  updatetask --id ID [--name \"NAME\"] [--start \"YYYY-MM-DD HH:MM\"] [--priority high|medium|low] [--category study|entertainment|life] [--remind \"YYYY-MM-DD HH:MM\"]\n"
        << "  showtask --all\n"
        << "  showtask --day YYYY-MM-DD\n"
        << "  showtask --month YYYY-MM\n"
        << "  deltask ID\n"
        << "  exit\n";
}

std::string CLI::readPassword(const std::string& prompt) {
    std::cout << prompt;
    std::cout.flush();

    TerminalEchoGuard echoGuard;
    const bool echoDisabled = echoGuard.disable();

    std::string password;
    std::getline(std::cin, password);

    if (echoDisabled) {
        std::cout << '\n';
    }

    return password;
}

// ==================== 功能：查看全部任务 ====================
void CLI::printTasks(const std::vector<Task>& tasks, std::ostream& output) {
    std::setlocale(LC_CTYPE, "");
    if (tasks.empty()) {
        output << "No tasks found.\n";
        return;
    }

    int lineWidth = 0;
    for (const auto& column : kTaskColumns) {
        printCell(output, column.title, column.width);
        lineWidth += column.width + 3;
    }
    output << '\n';
    output << std::string(static_cast<std::size_t>(lineWidth), '-') << '\n';

    for (const auto& task : tasks) {
        printCell(output, std::to_string(task.id), kTaskColumns[0].width);
        printCell(output, task.name, kTaskColumns[1].width);
        printCell(output, task.startTime, kTaskColumns[2].width);
        printCell(output, task.priority, kTaskColumns[3].width);
        printCell(output, task.category, kTaskColumns[4].width);
        printCell(output, task.remindTime, kTaskColumns[5].width);
        printCell(output, yesNo(task.reminded), kTaskColumns[6].width);
        output << '\n';
    }
}

bool CLI::parseAddTaskArgs(const std::vector<std::string>& args, Task& task, std::string& errorMessage) {
    // 将 addtask 命令选项转换为核心层 Task；默认值与核心层保持一致。
    for (std::size_t index = 0; index < args.size(); ++index) {
        const std::string& arg = args[index];
        if (arg == "--name" || arg == "--start" || arg == "--priority" || arg == "--category" || arg == "--remind") {
            if (index + 1 >= args.size()) {
                errorMessage = "Error: missing value for " + arg + '.';
                return false;
            }
        }

        if (arg == "--name") {
            task.name = args[++index];
        } else if (arg == "--start") {
            task.startTime = args[++index];
        } else if (arg == "--priority") {
            task.priority = args[++index];
        } else if (arg == "--category") {
            task.category = args[++index];
        } else if (arg == "--remind") {
            task.remindTime = args[++index];
        } else {
            errorMessage = "Error: unknown addtask option " + arg + '.';
            return false;
        }
    }

    if (task.name.empty() || task.startTime.empty()) {
        errorMessage = "Error: addtask requires --name and --start.";
        return false;
    }
    if (task.priority.empty()) {
        task.priority = "medium";
    }
    if (task.category.empty()) {
        task.category = "life";
    }
    if (task.remindTime.empty()) {
        task.remindTime = task.startTime;
    }

    return validateTaskFields(task, errorMessage);
}

bool CLI::parseUpdateTaskArgs(const std::vector<std::string>& args, UpdateTaskOptions& options, std::string& errorMessage) {
    // 只记录用户提供的字段，App 会先读取原任务再覆盖这些字段。
    bool hasId = false;
    for (std::size_t index = 0; index < args.size(); ++index) {
        const std::string& arg = args[index];
        if (arg == "--id" || arg == "--name" || arg == "--start" ||
            arg == "--priority" || arg == "--category" || arg == "--remind") {
            if (index + 1 >= args.size()) {
                errorMessage = "Error: missing value for " + arg + '.';
                return false;
            }
        }

        if (arg == "--id") {
            if (!parsePositiveTaskId(args[++index], options.taskId)) {
                errorMessage = "Error: invalid task id. Expected a positive integer.";
                return false;
            }
            hasId = true;
        } else if (arg == "--name") {
            options.values.name = args[++index];
            options.hasName = true;
        } else if (arg == "--start") {
            options.values.startTime = args[++index];
            options.hasStartTime = true;
        } else if (arg == "--priority") {
            options.values.priority = args[++index];
            options.hasPriority = true;
        } else if (arg == "--category") {
            options.values.category = args[++index];
            options.hasCategory = true;
        } else if (arg == "--remind") {
            options.values.remindTime = args[++index];
            options.hasRemindTime = true;
        } else {
            errorMessage = "Error: unknown updatetask option " + arg + '.';
            return false;
        }
    }

    if (!hasId) {
        errorMessage = "Error: updatetask requires --id <id>.";
        return false;
    }
    if (!options.hasName && !options.hasStartTime && !options.hasPriority &&
        !options.hasCategory && !options.hasRemindTime) {
        errorMessage = "Error: updatetask requires at least one field to update.";
        return false;
    }
    if (options.hasPriority && !isValidPriority(options.values.priority)) {
        errorMessage = "Error: invalid priority. Expected high, medium, or low.";
        return false;
    }
    if (options.hasCategory && !isValidCategory(options.values.category)) {
        errorMessage = "Error: invalid category. Expected study, entertainment, or life.";
        return false;
    }
    if (options.hasStartTime && !parseDateTime(options.values.startTime).has_value()) {
        errorMessage = "Error: invalid time format. Expected YYYY-MM-DD HH:MM.";
        return false;
    }
    if (options.hasRemindTime && !parseDateTime(options.values.remindTime).has_value()) {
        errorMessage = "Error: invalid remind time format. Expected YYYY-MM-DD HH:MM.";
        return false;
    }
    return true;
}

bool CLI::parseShowTaskArgs(const std::vector<std::string>& args, std::string& mode, std::string& value, std::string& errorMessage) {
    // showtask 支持三种查询模式：全部、某天、某月。
    if (args.size() == 1U && args[0] == "--all") {
        mode = "all";
        value.clear();
        return true;
    }
    if (args.size() == 2U && args[0] == "--day") {
        if (!isValidDayString(args[1])) {
            errorMessage = "Error: invalid day format. Expected YYYY-MM-DD.";
            return false;
        }
        mode = "day";
        value = args[1];
        return true;
    }
    if (args.size() == 2U && args[0] == "--month") {
        if (!isValidMonthString(args[1])) {
            errorMessage = "Error: invalid month format. Expected YYYY-MM.";
            return false;
        }
        mode = "month";
        value = args[1];
        return true;
    }

    errorMessage = "Error: showtask supports --all, --day YYYY-MM-DD, or --month YYYY-MM.";
    return false;
}

bool CLI::parseDeleteTaskArgs(const std::vector<std::string>& args, int& taskId, std::string& errorMessage) {
    // deltask 兼容 “deltask ID” 和 “deltask --id ID” 两种写法。
    const std::string* idValue = nullptr;
    if (args.size() == 1U) {
        idValue = &args[0];
    } else if (args.size() == 2U && args[0] == "--id") {
        idValue = &args[1];
    } else {
        errorMessage = "Error: deltask expects an id or --id <id>.";
        return false;
    }

    if (!parsePositiveTaskId(*idValue, taskId)) {
        errorMessage = "Error: invalid task id. Expected a positive integer.";
        return false;
    }
    return true;
}

bool CLI::promptForTask(Task& task, std::string& errorMessage) {
    // 交互式 addtask 的字段采集入口；返回前复用核心任务字段校验。
    std::cout << "Task name: ";
    std::getline(std::cin, task.name);
    task.name = trim(task.name);

    std::cout << "Start time [YYYY-MM-DD HH:MM]: ";
    std::getline(std::cin, task.startTime);
    task.startTime = trim(task.startTime);

    std::cout << "Priority [high/medium/low, default=medium]: ";
    std::getline(std::cin, task.priority);
    task.priority = trim(task.priority);
    if (task.priority.empty()) {
        task.priority = "medium";
    }

    std::cout << "Category [study/entertainment/life, default=life]: ";
    std::getline(std::cin, task.category);
    task.category = trim(task.category);
    if (task.category.empty()) {
        task.category = "life";
    }

    std::cout << "Remind time [YYYY-MM-DD HH:MM, default=start_time]: ";
    std::getline(std::cin, task.remindTime);
    task.remindTime = trim(task.remindTime);
    if (task.remindTime.empty()) {
        task.remindTime = task.startTime;
    }

    return validateTaskFields(task, errorMessage);
}
