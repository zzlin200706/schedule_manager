#ifndef MYSCHEDULE_TASK_H
#define MYSCHEDULE_TASK_H

#include <optional>
#include <string>

struct Task {
    int id = 0;
    std::string name;
    std::string startTime;
    std::string priority = "medium";
    std::string category = "life";
    std::string remindTime;
    bool reminded = false;
};

bool isValidPriority(const std::string& priority);
bool isValidCategory(const std::string& category);
bool validateTaskFields(const Task& task, std::string& errorMessage);
std::string serializeTask(const Task& task);
std::optional<Task> deserializeTask(const std::string& line, std::string& errorMessage);

#endif
