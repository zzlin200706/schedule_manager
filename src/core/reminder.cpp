#include "reminder.h"

/*
 * 文件职责：
 * 实现 CLI 模式下的后台提醒线程。线程定期调用 TaskManager 收集到期任务，
 * 通过回调输出提醒，并复用 AudioAlert 播放提醒音频。
 *
 * 主要功能定位：
 * - 功能：提醒线程
 */

#include "audio_alert.h"

#include <chrono>
#include <utility>

Reminder::Reminder(TaskManager& taskManager, ReminderCallback onReminder, ErrorCallback onError)
    : taskManager_(taskManager),
      onReminder_(std::move(onReminder)),
      onError_(std::move(onError)) {}

Reminder::~Reminder() {
    stop();
}

void Reminder::start() {
    // start 可重复调用；running_ 已为 true 时不再创建第二个线程。
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
    // stop 将运行标志置 false，并等待 worker_ 退出，避免析构后线程继续访问对象。
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
        // 步骤 1：向核心层查询到期任务；核心层负责标记 reminded 并保存。
        if (taskManager_.collectDueReminders(reminders, message)) {
            for (const auto& task : reminders) {
                if (onReminder_) {
                    onReminder_(task);
                }
            }
            if (!reminders.empty()) {
                // 步骤 2：同一轮检查中只要存在到期任务，就触发一次异步音频播放。
                AudioAlert::playReminderSoundAsync();
            }
        } else if (!message.empty()) {
            if (onError_) {
                onError_(message);
            }
        }

        // 步骤 3：每 10 秒检查一次；拆成 1 秒循环便于 stop 更快生效。
        for (int second = 0; second < 10 && running_; ++second) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
}
