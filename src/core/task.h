#ifndef MYSCHEDULE_TASK_H
#define MYSCHEDULE_TASK_H

/*
 * 文件职责：
 * 定义核心层任务数据结构和任务字段校验/序列化接口。
 * TaskManager、Storage、CLI 和 Qt 适配层都会使用这里的标准任务模型。
 */

#include <optional>
#include <string>

/**
 * @brief 核心层任务模型
 *
 * id 由 TaskManager 分配；priority/category 在核心层有固定允许值；
 * reminded 用于记录该任务是否已经触发过提醒。
 */
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
