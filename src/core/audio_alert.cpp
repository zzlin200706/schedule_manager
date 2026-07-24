#include "audio_alert.h"

#include <atomic>
#include <cstdlib>
#include <filesystem>
#include <system_error>
#include <thread>

namespace {

constexpr const char* kReminderSoundPath = "resources/alarm.mp3";
std::atomic_bool playbackInProgress {false};

}

namespace AudioAlert {

const std::string& reminderSoundPath()
{
    static const std::string path = kReminderSoundPath;
    return path;
}

bool canPlayReminderSound(std::string& message)
{
    std::error_code error;
    if (!std::filesystem::is_regular_file(reminderSoundPath(), error)) {
        message = "Reminder sound file not found: " + reminderSoundPath();
        return false;
    }
    if (std::system("command -v mpg123 >/dev/null 2>&1") != 0) {
        message = "mpg123 is not available.";
        return false;
    }

    message.clear();
    return true;
}

bool playReminderSoundBlocking(std::string& message)
{
    if (!canPlayReminderSound(message)) {
        return false;
    }
    if (std::system("mpg123 resources/alarm.mp3 >/dev/null 2>&1") != 0) {
        message = "Failed to play reminder sound.";
        return false;
    }

    message.clear();
    return true;
}

void playReminderSoundAsync()
{
    bool expected = false;
    if (!playbackInProgress.compare_exchange_strong(expected, true)) {
        return;
    }

    try {
        std::thread([] {
            std::string ignoredMessage;
            playReminderSoundBlocking(ignoredMessage);
            playbackInProgress = false;
        }).detach();
    } catch (...) {
        playbackInProgress = false;
    }
}

}
