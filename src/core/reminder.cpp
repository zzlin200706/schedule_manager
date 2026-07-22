#include "reminder.h"

#include <chrono>
#include <utility>
#include<cstdlib>

namespace {

void playReminderSound()
{
    system("mpg123 resources/alarm.mp3 >/dev/null 2>&1");
}

}

Reminder::Reminder(TaskManager& taskManager, ReminderCallback onReminder, ErrorCallback onError)
    : taskManager_(taskManager),
      onReminder_(std::move(onReminder)),
      onError_(std::move(onError)) {}

Reminder::~Reminder() {
    stop();
}

void Reminder::start() {
    if (running_.exchange(true)) {
        return;
    }
    try {
        worker_ = std::thread(&Reminder::run, this);
    } catch (...) {
        running_ = false;
        throw;
    }
}

void Reminder::stop() {
    if (!running_.exchange(false)) {
        return;
    }
    if (worker_.joinable()) {
        worker_.join();
    }
}

void Reminder::run() {
    while (running_) {
        std::vector<Task> reminders;
        std::string message;
        if (taskManager_.collectDueReminders(reminders, message)) {
            for (const auto& task : reminders) {
                if (onReminder_) {
                    onReminder_(task);
                }

		playReminderSound();
            }
        } else if (!message.empty()) {
            if (onError_) {
                onError_(message);
            }
        }

        for (int second = 0; second < 10 && running_; ++second) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
}
