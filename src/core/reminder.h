#ifndef MYSCHEDULE_REMINDER_H
#define MYSCHEDULE_REMINDER_H

#include "task_manager.h"

#include <atomic>
#include <functional>
#include <string>
#include <thread>

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
