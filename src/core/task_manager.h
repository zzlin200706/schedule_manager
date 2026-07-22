#ifndef MYSCHEDULE_TASK_MANAGER_H
#define MYSCHEDULE_TASK_MANAGER_H

#include "task.h"

#include <mutex>
#include <string>
#include <vector>

class TaskManager {
public:
    explicit TaskManager(std::string username);

    bool load(std::string& message);
    bool addTask(Task task, std::string& message);
    bool updateTask(int taskId, const Task& updatedTask, std::string& message);
    bool deleteTaskById(int id, std::string& message);
    std::vector<Task> getAllTasks() const;
    std::vector<Task> getTasksByDay(const std::string& day) const;
    std::vector<Task> getTasksByMonth(const std::string& month) const;
    int generateNextId() const;

private:
    std::string username_;
    std::vector<Task> tasks_;
    int nextId_ = 1;
    mutable std::mutex mutex_;

    bool saveUnlocked(std::string& message) const;
    bool hasStartTimeConflictUnlocked(const Task& task, int ignoredId = -1) const;
    bool hasNameAndStartConflictUnlocked(const Task& task, int ignoredId = -1) const;
    int generateNextIdUnlocked() const;
    void sortTasksUnlocked();
};

#endif
