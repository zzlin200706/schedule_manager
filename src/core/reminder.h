#ifndef MYSCHEDULE_REMINDER_H
#define MYSCHEDULE_REMINDER_H

/*
 * 文件职责：
 * 声明 CLI 使用的后台提醒线程封装。Qt GUI 使用自己的 QTimer 轮询，
 * 但提醒判断仍复用 TaskManager::collectDueReminders。
 */

#include "task_manager.h"

#include <atomic>
#include <functional>
#include <string>
#include <thread>

/**
 * @brief 后台提醒线程
 *
 * start() 后每隔一段时间检查核心层到期提醒，通过回调把到期任务交给调用层，
 * stop() 或析构时停止线程。
 */
class Reminder {
public:
    using ReminderCallback = std::function<void(const Task&)>;
    using ErrorCallback = std::function<void(const std::string&)>;

    explicit Reminder(TaskManager& taskManager,
                      ReminderCallback onReminder = {},
                      ErrorCallback onError = {});
    ~Reminder();

    void start();
    void stop();

private:
    void run();

    TaskManager& taskManager_;
    ReminderCallback onReminder_;
    ErrorCallback onError_;
    std::atomic<bool> running_{false};
    std::thread worker_;
};

#endif
