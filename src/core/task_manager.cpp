#include "task_manager.h"

/*
 * 文件职责：
 * 实现日程任务核心管理逻辑，包括加载、添加、删除、修改、查询、
 * 排序、唯一性检查和提醒状态更新。
 *
 * 主要功能定位：
 * - 功能：加载用户任务
 * - 功能：添加任务
 * - 功能：删除任务
 * - 功能：修改任务
 * - 功能：查看全部任务 / 按日期查看任务 / 按月份查看任务
 * - 功能：任务排序
 */

#include "storage.h"
#include "time_utils.h"

#include <algorithm>
#include <ctime>
#include <limits>
#include <unordered_set>
#include <utility>

TaskManager::TaskManager(std::string username) : username_(std::move(username)) {}

// ==================== 功能：加载用户任务 ====================
bool TaskManager::load(std::string& message) {
    std::string errorMessage;
    TaskFileData loaded = Storage::loadTasks(username_, errorMessage);
    if (!errorMessage.empty()) {
        message = errorMessage;
        return false;
    }

    std::unordered_set<int> ids;
    std::unordered_set<std::string> startTimes;
    int maxId = 0;
    // 步骤 1：校验存储文件中的 ID 和开始时间，避免加载出重复任务。
    for (const auto& task : loaded.tasks) {
        if (!ids.insert(task.id).second) {
            message = "Error: duplicate task id in storage.";
            return false;
        }
        if (!startTimes.insert(task.startTime).second) {
            message = "Error: duplicate task start time in storage.";
            return false;
        }
        maxId = std::max(maxId, task.id);
    }
    if (maxId == std::numeric_limits<int>::max()) {
        message = "Error: task id range is exhausted.";
        return false;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    // 步骤 2：写入内存缓存，并根据文件元数据和现有最大 ID 恢复 nextId。
    tasks_ = std::move(loaded.tasks);
    nextId_ = std::max(loaded.nextId, maxId + 1);
    sortTasksUnlocked();
    message = "Tasks loaded successfully.";
    return true;
}

// ==================== 功能：添加任务 ====================
bool TaskManager::addTask(Task task, std::string& message) {
    // 步骤 1：补齐默认字段；CLI/Qt 可省略这些值，核心层统一兜底。
    if (task.priority.empty()) {
        task.priority = "medium";
    }
    if (task.category.empty()) {
        task.category = "life";
    }
    if (task.remindTime.empty()) {
        task.remindTime = task.startTime;
    }
    task.reminded = false;

    // 步骤 2：校验字段格式和枚举值，例如时间格式、优先级、分类。
    std::string validationError;
    if (!validateTaskFields(task, validationError)) {
        message = validationError;
        return false;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    // 步骤 3：检查开始时间唯一，以及“名称 + 开始时间”组合唯一。
    if (hasStartTimeConflictUnlocked(task)) {
        message = "Error: task start time already exists.";
        return false;
    }
    if (hasNameAndStartConflictUnlocked(task)) {
        message = "Error: task name plus start time must be unique.";
        return false;
    }

    if (nextId_ == std::numeric_limits<int>::max()) {
        message = "Error: task id range is exhausted.";
        return false;
    }

    const int originalNextId = nextId_;
    // 步骤 4：分配唯一 ID，加入内存列表，排序后保存到用户任务文件。
    task.id = nextId_++;
    tasks_.push_back(task);
    sortTasksUnlocked();
    if (!saveUnlocked(message)) {
        // 保存失败时回滚内存和 nextId，避免内存状态与文件状态不一致。
        tasks_.erase(std::remove_if(tasks_.begin(), tasks_.end(), [&](const Task& existing) {
            return existing.id == task.id;
        }), tasks_.end());
        nextId_ = originalNextId;
        return false;
    }

    message = "Task added successfully. ID = " + std::to_string(task.id);
    return true;
}

// ==================== 功能：修改任务 ====================
bool TaskManager::updateTask(int taskId, const Task& updatedTask, std::string& message) {
    std::lock_guard<std::mutex> lock(mutex_);
    // 步骤 1：先按真实任务 ID 定位原任务；不存在的 ID 直接失败。
    const auto it = std::find_if(tasks_.begin(), tasks_.end(), [&](const Task& task) {
        return task.id == taskId;
    });
    if (it == tasks_.end()) {
        message = "Error: task id " + std::to_string(taskId) + " not found.";
        return false;
    }

    // 步骤 2：在原任务副本上覆盖允许修改的字段，保留业务上不可由外部改动的 ID。
    Task replacement = *it;
    replacement.id = taskId;
    replacement.name = updatedTask.name;
    replacement.startTime = updatedTask.startTime;
    replacement.priority = updatedTask.priority;
    replacement.category = updatedTask.category;
    replacement.remindTime = updatedTask.remindTime.empty() ? updatedTask.startTime : updatedTask.remindTime;
    if (replacement.startTime != it->startTime || replacement.remindTime != it->remindTime) {
        replacement.reminded = false;
    }

    // 步骤 3：重新执行字段校验和唯一性检查，忽略当前被修改的任务 ID。
    std::string validationError;
    if (!validateTaskFields(replacement, validationError)) {
        message = validationError;
        return false;
    }
    if (hasStartTimeConflictUnlocked(replacement, taskId)) {
        message = "Error: task start time already exists.";
        return false;
    }
    if (hasNameAndStartConflictUnlocked(replacement, taskId)) {
        message = "Error: task name plus start time must be unique.";
        return false;
    }

    const std::vector<Task> originalTasks = tasks_;
    // 步骤 4：写入内存、排序并保存；保存失败则恢复旧任务列表。
    *it = replacement;
    sortTasksUnlocked();
    if (!saveUnlocked(message)) {
        tasks_ = originalTasks;
        return false;
    }

    message = "Task updated successfully.";
    return true;
}

// ==================== 功能：删除任务 ====================
bool TaskManager::deleteTaskById(int id, std::string& message) {
    std::lock_guard<std::mutex> lock(mutex_);
    // 步骤 1：根据 ID 查找任务；非法或不存在的 ID 不会修改内存和文件。
    const auto it = std::find_if(tasks_.begin(), tasks_.end(), [&](const Task& task) {
        return task.id == id;
    });
    if (it == tasks_.end()) {
        message = "Error: task id " + std::to_string(id) + " not found.";
        return false;
    }

    const std::vector<Task> originalTasks = tasks_;
    // 步骤 2：删除后立即保存；保存失败则恢复删除前的列表。
    tasks_.erase(it);
    if (!saveUnlocked(message)) {
        tasks_ = originalTasks;
        return false;
    }
    message = "Task deleted successfully.";
    return true;
}

// ==================== 功能：查看全部任务 ====================
std::vector<Task> TaskManager::getAllTasks() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return tasks_;
}

// ==================== 功能：按日期查看任务 ====================
std::vector<Task> TaskManager::getTasksByDay(const std::string& day) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<Task> filtered;
    for (const auto& task : tasks_) {
        if (sameDay(task.startTime, day)) {
            filtered.push_back(task);
        }
    }
    return filtered;
}

// ==================== 功能：按月份查看任务 ====================
std::vector<Task> TaskManager::getTasksByMonth(const std::string& month) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<Task> filtered;
    for (const auto& task : tasks_) {
        if (sameMonth(task.startTime, month)) {
            filtered.push_back(task);
        }
    }
    return filtered;
}


// ==================== 功能：提醒线程 ====================
bool TaskManager::collectDueReminders(std::vector<Task>& reminders, std::string& message) {
    std::lock_guard<std::mutex> lock(mutex_);
    const std::time_t now = std::time(nullptr);
    const std::vector<Task> originalTasks = tasks_;
    bool changed = false;

    for (auto& task : tasks_) {
        const auto remindTm = parseDateTime(task.remindTime);
        if (!remindTm.has_value()) {
            continue;
        }
        if (!task.reminded && toTimeT(*remindTm) <= now) {
            // 到达提醒时间后立即标记 reminded，后续保存可避免重复提醒。
            task.reminded = true;
            reminders.push_back(task);
            changed = true;
        }
    }

    if (changed && !saveUnlocked(message)) {
        tasks_ = originalTasks;
        reminders.clear();
        return false;
    }

    if (!changed) {
        message.clear();
    }
    return true;
}

int TaskManager::generateNextId() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return generateNextIdUnlocked();
}

bool TaskManager::saveUnlocked(std::string& message) const {
    std::string errorMessage;
    if (!Storage::saveTasks(username_, tasks_, nextId_, errorMessage)) {
        message = errorMessage;
        return false;
    }
    return true;
}

bool TaskManager::hasStartTimeConflictUnlocked(const Task& task, int ignoredId) const {
    return std::any_of(tasks_.begin(), tasks_.end(), [&](const Task& existing) {
        return existing.id != ignoredId && existing.startTime == task.startTime;
    });
}

bool TaskManager::hasNameAndStartConflictUnlocked(const Task& task, int ignoredId) const {
    return std::any_of(tasks_.begin(), tasks_.end(), [&](const Task& existing) {
        return existing.id != ignoredId &&
               existing.name == task.name &&
               existing.startTime == task.startTime;
    });
}

int TaskManager::generateNextIdUnlocked() const {
    return nextId_;
}

// ==================== 功能：任务排序 ====================
void TaskManager::sortTasksUnlocked() {
    std::sort(tasks_.begin(), tasks_.end(), [](const Task& left, const Task& right) {
        return compareDateTimes(left.startTime, right.startTime) < 0;
    });
}
