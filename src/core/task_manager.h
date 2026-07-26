#ifndef MYSCHEDULE_TASK_MANAGER_H
#define MYSCHEDULE_TASK_MANAGER_H

/*
 * 文件职责：
 * 声明日程任务的核心管理类，是 CLI 和 Qt GUI 共同调用的任务业务入口。
 *
 * 主要功能定位：
 * - 添加任务：搜索“功能：添加任务”
 * - 删除任务：搜索“功能：删除任务”
 * - 修改任务：搜索“功能：修改任务”
 * - 提醒检查：搜索“功能：提醒线程”
 */

#include "task.h"

#include <mutex>
#include <string>
#include <vector>

/**
 * @brief 当前用户的任务管理核心类
 *
 * 负责维护某个用户的任务列表、下一个任务 ID、排序规则和保存/加载入口。
 * GUI/CLI 不直接修改数据文件，而是通过本类完成增删改查和提醒状态更新。
 */
class TaskManager {
public:
    explicit TaskManager(std::string username);

    /**
     * @brief 加载当前用户任务
     *
     * 从 Storage 读取用户任务文件，校验任务 ID 和开始时间唯一性，
     * 并设置后续新增任务使用的 nextId。
     */
    bool load(std::string& message);

    /**
     * @brief 添加一条任务并立即保存
     *
     * 这里补齐默认优先级、默认分类和默认提醒时间，检查开始时间冲突、
     * 名称与开始时间组合冲突，然后分配唯一 ID。
     */
    bool addTask(Task task, std::string& message);

    /**
     * @brief 修改指定 ID 的任务并立即保存
     *
     * 可修改名称、开始时间、优先级、分类和提醒时间；修改后仍会复用
     * 核心层的字段校验和唯一性检查。
     */
    bool updateTask(int taskId, const Task& updatedTask, std::string& message);

    /**
     * @brief 按任务 ID 删除任务并立即保存
     */
    bool deleteTaskById(int id, std::string& message);

    std::vector<Task> getAllTasks() const;
    std::vector<Task> getTasksByDay(const std::string& day) const;
    std::vector<Task> getTasksByMonth(const std::string& month) const;

    /**
     * @brief 收集已经到达提醒时间的任务
     *
     * 会把已提醒任务的 reminded 字段写回文件，避免同一任务被重复提醒。
     */
    bool collectDueReminders(std::vector<Task>& reminders, std::string& message);
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
