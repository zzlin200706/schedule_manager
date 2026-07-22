#include "task_manager.h"

#include "storage.h"
#include "time_utils.h"

#include <algorithm>
#include <limits>
#include <unordered_set>
#include <utility>
#include<ctime>
TaskManager::TaskManager(std::string username) : username_(std::move(username)) {}

bool TaskManager::load(std::string& message) {
    std::string errorMessage;
    std::vector<Task> loaded = Storage::loadTasks(username_, errorMessage);
    if (!errorMessage.empty()) {
        message = errorMessage;
        return false;
    }

    std::unordered_set<int> ids;
    std::unordered_set<std::string> startTimes;
    int maxId = 0;
    for (const auto& task : loaded) {
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
    tasks_ = std::move(loaded);
    nextId_ = maxId + 1;
    sortTasksUnlocked();
    message = "Tasks loaded successfully.";
    return true;
}

bool TaskManager::addTask(Task task, std::string& message) {
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

    std::string validationError;
    if (!validateTaskFields(task, validationError)) {
        message = validationError;
        return false;
    }

    std::lock_guard<std::mutex> lock(mutex_);
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

    task.id = nextId_++;
    tasks_.push_back(task);
    sortTasksUnlocked();
    if (!saveUnlocked(message)) {
        return false;
    }

    message = "Task added successfully. ID = " + std::to_string(task.id);
    return true;
}

bool TaskManager::updateTask(int taskId, const Task& updatedTask, std::string& message) {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto it = std::find_if(tasks_.begin(), tasks_.end(), [&](const Task& task) {
        return task.id == taskId;
    });
    if (it == tasks_.end()) {
        message = "Error: task id " + std::to_string(taskId) + " not found.";
        return false;
    }

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

    *it = replacement;
    sortTasksUnlocked();
    if (!saveUnlocked(message)) {
        return false;
    }

    message = "Task updated successfully.";
    return true;
}

bool TaskManager::deleteTaskById(int id, std::string& message) {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto it = std::find_if(tasks_.begin(), tasks_.end(), [&](const Task& task) {
        return task.id == id;
    });
    if (it == tasks_.end()) {
        message = "Error: task id " + std::to_string(id) + " not found.";
        return false;
    }

    tasks_.erase(it);
    if (!saveUnlocked(message)) {
        return false;
    }
    message = "Task deleted successfully.";
    return true;
}

std::vector<Task> TaskManager::getAllTasks() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return tasks_;
}

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

std::vector<Task> TaskManager::getDueReminders() const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<Task> result;

    std::time_t now = std::time(nullptr);

    for (const auto& task : tasks_) {
        if (!task.reminded && !task.remindTime.empty()) {
            auto remind = parseDateTime(task.remindTime);

            if (remind.has_value() &&
                std::difftime(std::mktime(&remind.value()),
                              now) <= 0) {
                result.push_back(task);
            }
        }
    }

    return result;
}

int TaskManager::generateNextId() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return generateNextIdUnlocked();
}

bool TaskManager::saveUnlocked(std::string& message) const {
    std::string errorMessage;
    if (!Storage::saveTasks(username_, tasks_, errorMessage)) {
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

void TaskManager::sortTasksUnlocked() {
    std::sort(tasks_.begin(), tasks_.end(), [](const Task& left, const Task& right) {
        return compareDateTimes(left.startTime, right.startTime) < 0;
    });
}
