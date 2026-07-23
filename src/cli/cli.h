#ifndef MYSCHEDULE_CLI_H
#define MYSCHEDULE_CLI_H

#include "task.h"

#include <ostream>
#include <string>
#include <vector>

struct UpdateTaskOptions {
    int taskId = 0;
    bool hasName = false;
    bool hasStartTime = false;
    bool hasPriority = false;
    bool hasCategory = false;
    bool hasRemindTime = false;
    Task values;
};

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
