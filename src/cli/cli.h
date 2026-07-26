#ifndef MYSCHEDULE_CLI_H
#define MYSCHEDULE_CLI_H

/*
 * 文件职责：
 * 声明 CLI 输出、密码读取和命令参数解析工具。
 * App 负责命令分发，本类只负责把用户输入转换成核心层可用的数据结构。
 */

#include "task.h"

#include <ostream>
#include <string>
#include <vector>

/**
 * @brief updatetask 参数解析结果
 *
 * hasXxx 表示用户是否显式传入某个字段，values 保存对应的新值。
 */
struct UpdateTaskOptions {
    int taskId = 0;
    bool hasName = false;
    bool hasStartTime = false;
    bool hasPriority = false;
    bool hasCategory = false;
    bool hasRemindTime = false;
    Task values;
};

/**
 * @brief CLI 参数解析和输出工具类
 */
class CLI {
public:
    static void printHelp(std::ostream& output);
    static void printInteractiveHelp(std::ostream& output);
    static void printTasks(const std::vector<Task>& tasks, std::ostream& output);
    static std::string readPassword(const std::string& prompt);

    static bool parseAddTaskArgs(const std::vector<std::string>& args, Task& task, std::string& errorMessage);
    static bool parseUpdateTaskArgs(const std::vector<std::string>& args, UpdateTaskOptions& options, std::string& errorMessage);
    static bool parseShowTaskArgs(const std::vector<std::string>& args, std::string& mode, std::string& value, std::string& errorMessage);
    static bool parseDeleteTaskArgs(const std::vector<std::string>& args, int& taskId, std::string& errorMessage);
    static bool promptForTask(Task& task, std::string& errorMessage);
};

#endif
