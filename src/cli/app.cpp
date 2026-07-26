#include "app.h"

/*
 * 文件职责：
 * 实现 CLI 的登录/注册流程、交互式命令循环、直接命令执行和提醒线程启动。
 * 本文件负责“命令从哪里进入”，具体参数解析在 cli.cpp，核心业务在 TaskManager/UserManager。
 *
 * 主要功能定位：
 * - 功能：CLI 命令分发
 * - 功能：用户登录
 * - 功能：用户注册
 * - 功能：提醒线程
 */

#include "cli.h"
#include "reminder.h"
#include "task_manager.h"
#include "time_utils.h"
#include "user.h"

#include <algorithm>
#include <iostream>
#include <system_error>

namespace {
void printErrorWithHelp(const std::string& message) {
    std::cerr << message << '\n';
    std::cerr << "Run `myschedule --help` for usage.\n";
}

bool buildUpdatedTask(TaskManager& taskManager,
                      const UpdateTaskOptions& options,
                      Task& updatedTask,
                      std::string& message) {
    // 修改命令只携带被改字段；这里先取出原任务，再覆盖用户显式提供的字段。
    const auto tasks = taskManager.getAllTasks();
    const auto it = std::find_if(tasks.begin(), tasks.end(), [&](const Task& task) {
        return task.id == options.taskId;
    });
    if (it == tasks.end()) {
        message = "Error: task id " + std::to_string(options.taskId) + " not found.";
        return false;
    }

    updatedTask = *it;
    if (options.hasName) {
        updatedTask.name = options.values.name;
    }
    if (options.hasStartTime) {
        updatedTask.startTime = options.values.startTime;
    }
    if (options.hasPriority) {
        updatedTask.priority = options.values.priority;
    }
    if (options.hasCategory) {
        updatedTask.category = options.values.category;
    }
    if (options.hasRemindTime) {
        updatedTask.remindTime = options.values.remindTime;
    }
    return true;
}
}

// ==================== 功能：CLI 命令分发 ====================
int App::run(const std::vector<std::string>& args) {
    if (args.empty()) {
        printErrorWithHelp("Error: no command was provided.");
        return 1;
    }

    if (args[0] == "--help" || args[0] == "help") {
        CLI::printHelp(std::cout);
        return 0;
    }

    if (args[0] == "run") {
        if (args.size() != 1U) {
            printErrorWithHelp("Error: run does not accept extra arguments.");
            return 1;
        }
        return runInteractiveMode();
    }

    if (args.size() < 3U) {
        printErrorWithHelp("Error: missing username, password, or command.");
        return 1;
    }
    return runAuthenticatedCommand(args);
}

int App::runInteractiveMode() {
    UserManager userManager;
    std::string username;
    std::string password;

    // ==================== 功能：用户登录 ====================
    // 步骤 1：从终端读取用户名和密码；密码读取由 CLI::readPassword 关闭回显。
    std::cout << "Welcome to MySchedule!\n\n";
    std::cout << "Username: ";
    std::getline(std::cin, username);
    username = trim(username);

    password = CLI::readPassword("Password: ");

    if (username.empty() || password.empty()) {
        std::cerr << "Error: username and password cannot be empty.\n";
        return 1;
    }

    std::string message;
    if (!userManager.loadSucceeded(message)) {
        std::cerr << message << '\n';
        return 1;
    }
    if (!userManager.userExists(username)) {
        // ==================== 功能：用户注册 ====================
        // 交互模式下用户名不存在时，询问是否直接创建账号。
        std::cout << "User not found. Create new account? [y/N]: ";
        std::string answer;
        std::getline(std::cin, answer);
        answer = trim(answer);
        if (answer != "y" && answer != "Y") {
            std::cerr << "Account creation cancelled.\n";
            return 1;
        }
        if (!userManager.registerUser(username, password, message)) {
            std::cerr << message << '\n';
            return 1;
        }
        std::cout << message << '\n';
    } else if (!userManager.login(username, password, message)) {
        // 已存在用户走核心层登录校验；登录成功后再加载该用户任务。
        std::cerr << message << '\n';
        return 1;
    }

    // 步骤 2：登录/注册成功后创建当前用户的 TaskManager 并加载任务文件。
    TaskManager taskManager(username);
    if (!taskManager.load(message)) {
        std::cerr << message << '\n';
        return 1;
    }

    Reminder reminder(
        taskManager,
        [](const Task& task) {
            std::cout << "\n[Reminder] "
                      << task.name
                      << " starts at "
                      << task.startTime
                      << '.' << std::endl;
        },
        [](const std::string& error) {
            std::cerr << error << std::endl;
        });
    try {
        // ==================== 功能：提醒线程 ====================
        // 交互式 CLI 登录成功后启动后台提醒线程，退出循环前显式停止。
        reminder.start();
    } catch (const std::system_error& error) {
        std::cerr << "Error: failed to start reminder thread. " << error.what() << '\n';
        return 1;
    }

    CLI::printInteractiveHelp(std::cout);
    while (true) {
        // 步骤 3：读取一行交互式命令，并用 time_utils.cpp 的分词器处理引号和转义。
        std::cout << "myschedule> ";
        std::string line;
        if (!std::getline(std::cin, line)) {
            break;
        }
        line = trim(line);
        if (line.empty()) {
            continue;
        }

        std::vector<std::string> tokens;
        if (!tokenizeCommandLine(line, tokens, message)) {
            std::cerr << message << '\n';
            continue;
        }
        if (tokens.empty()) {
            continue;
        }

        if (tokens[0] == "help") {
            CLI::printInteractiveHelp(std::cout);
            continue;
        }
        if (tokens[0] == "exit") {
            break;
        }
        if (tokens[0] == "addtask") {
            // addtask 交互式入口：逐项提示输入，再调用 TaskManager::addTask 保存。
            Task task;
            if (!CLI::promptForTask(task, message)) {
                std::cerr << message << '\n';
                continue;
            }
            if (!taskManager.addTask(task, message)) {
                std::cerr << message << '\n';
                continue;
            }
            std::cout << message << '\n';
            continue;
        }
        if (tokens[0] == "updatetask") {
            // updatetask 入口：解析可选字段，合成完整任务后调用核心层修改。
            UpdateTaskOptions options;
            if (!CLI::parseUpdateTaskArgs(std::vector<std::string>(tokens.begin() + 1, tokens.end()), options, message)) {
                std::cerr << message << '\n';
                continue;
            }
            Task updatedTask;
            if (!buildUpdatedTask(taskManager, options, updatedTask, message)) {
                std::cerr << message << '\n';
                continue;
            }
            if (!taskManager.updateTask(options.taskId, updatedTask, message)) {
                std::cerr << message << '\n';
                continue;
            }
            std::cout << message << '\n';
            continue;
        }
        if (tokens[0] == "showtask") {
            // showtask 入口：根据 --all/--day/--month 选择核心层查询接口。
            std::string mode;
            std::string value;
            if (!CLI::parseShowTaskArgs(std::vector<std::string>(tokens.begin() + 1, tokens.end()), mode, value, message)) {
                std::cerr << message << '\n';
                continue;
            }
            if (mode == "all") {
                CLI::printTasks(taskManager.getAllTasks(), std::cout);
            } else if (mode == "day") {
                CLI::printTasks(taskManager.getTasksByDay(value), std::cout);
            } else {
                CLI::printTasks(taskManager.getTasksByMonth(value), std::cout);
            }
            continue;
        }
        if (tokens[0] == "deltask") {
            // deltask 入口：解析任务 ID 后调用核心层按 ID 删除。
            int taskId = 0;
            if (!CLI::parseDeleteTaskArgs(std::vector<std::string>(tokens.begin() + 1, tokens.end()), taskId, message)) {
                std::cerr << message << '\n';
                continue;
            }
            if (!taskManager.deleteTaskById(taskId, message)) {
                std::cerr << message << '\n';
                continue;
            }
            std::cout << message << '\n';
            continue;
        }

        std::cerr << "Error: unknown command.\n";
    }

    reminder.stop();
    return 0;
}

int App::runAuthenticatedCommand(const std::vector<std::string>& args) {
    // 直接命令模式：myschedule <username> <password> <command> ...
    // 与交互模式共用 UserManager/TaskManager，只是不启动后台提醒线程。
    UserManager userManager;
    const std::string username = args[0];
    const std::string password = args[1];
    const std::string command = args[2];
    std::string message;

    // ==================== 功能：用户登录 ====================
    if (!userManager.login(username, password, message)) {
        std::cerr << message << '\n';
        return 1;
    }

    // 认证后加载任务，后续 add/update/delete 会由核心层自动保存。
    TaskManager taskManager(username);
    if (!taskManager.load(message)) {
        std::cerr << message << '\n';
        return 1;
    }

    if (command == "addtask") {
        // 直接 addtask：命令行参数 -> Task -> TaskManager::addTask。
        Task task;
        if (!CLI::parseAddTaskArgs(std::vector<std::string>(args.begin() + 3, args.end()), task, message)) {
            std::cerr << message << '\n';
            return 1;
        }
        if (!taskManager.addTask(task, message)) {
            std::cerr << message << '\n';
            return 1;
        }
        std::cout << message << '\n';
        return 0;
    }

    if (command == "updatetask") {
        // 直接 updatetask：参数只表示要改的字段，buildUpdatedTask 合成完整任务。
        UpdateTaskOptions options;
        if (!CLI::parseUpdateTaskArgs(std::vector<std::string>(args.begin() + 3, args.end()), options, message)) {
            std::cerr << message << '\n';
            return 1;
        }
        Task updatedTask;
        if (!buildUpdatedTask(taskManager, options, updatedTask, message)) {
            std::cerr << message << '\n';
            return 1;
        }
        if (!taskManager.updateTask(options.taskId, updatedTask, message)) {
            std::cerr << message << '\n';
            return 1;
        }
        std::cout << message << '\n';
        return 0;
    }

    if (command == "showtask") {
        // 直接 showtask：显示全部、指定日期或指定月份任务。
        std::string mode;
        std::string value;
        if (!CLI::parseShowTaskArgs(std::vector<std::string>(args.begin() + 3, args.end()), mode, value, message)) {
            std::cerr << message << '\n';
            return 1;
        }
        if (mode == "all") {
            CLI::printTasks(taskManager.getAllTasks(), std::cout);
        } else if (mode == "day") {
            CLI::printTasks(taskManager.getTasksByDay(value), std::cout);
        } else {
            CLI::printTasks(taskManager.getTasksByMonth(value), std::cout);
        }
        return 0;
    }

    if (command == "deltask") {
        // 直接 deltask：只接受真实任务 ID，删除成功后立即保存。
        int taskId = 0;
        if (!CLI::parseDeleteTaskArgs(std::vector<std::string>(args.begin() + 3, args.end()), taskId, message)) {
            std::cerr << message << '\n';
            return 1;
        }
        if (!taskManager.deleteTaskById(taskId, message)) {
            std::cerr << message << '\n';
            return 1;
        }
        std::cout << message << '\n';
        return 0;
    }

    printErrorWithHelp("Error: unknown command.");
    return 1;
}
