#include "audio_alert.h"

/*
 * 文件职责：
 * 实现提醒音频查找和播放。当前通过系统命令 mpg123 播放
 * resources/alarm.mp3，提供同步检测/播放和异步播放入口。
 *
 * 主要功能定位：
 * - 功能：播放提醒音频
 */

#include <atomic>
#include <cstdlib>
#include <filesystem>
#include <system_error>
#include <thread>

namespace {

constexpr const char* kReminderSoundPath = "resources/alarm.mp3";
std::atomic_bool playbackInProgress {false};

bool isReminderSoundFile(const std::filesystem::path& path)
{
    std::error_code error;
    return std::filesystem::is_regular_file(path, error);
}

std::filesystem::path executableDirectory()
{
    std::error_code error;
    const std::filesystem::path executablePath =
        std::filesystem::read_symlink("/proc/self/exe", error);
    if (error || executablePath.empty()) {
        return {};
    }
    return executablePath.parent_path();
}

std::filesystem::path findReminderSoundPath()
{
    const std::filesystem::path relativeSoundPath(kReminderSoundPath);

    // 兼容从项目根目录、build 目录或 GUI 可执行文件目录启动。
    if (isReminderSoundFile(relativeSoundPath)) {
        return relativeSoundPath;
    }

    std::error_code error;
    const std::filesystem::path currentPath = std::filesystem::current_path(error);
    if (!error && !currentPath.empty()) {
        const std::filesystem::path fromParent = currentPath.parent_path() / relativeSoundPath;
        if (isReminderSoundFile(fromParent)) {
            return fromParent;
        }
    }

    const std::filesystem::path executablePath = executableDirectory();
    if (!executablePath.empty()) {
        const std::filesystem::path fromExecutable = executablePath / relativeSoundPath;
        if (isReminderSoundFile(fromExecutable)) {
            return fromExecutable;
        }

        const std::filesystem::path fromExecutableParent =
            executablePath.parent_path() / relativeSoundPath;
        if (isReminderSoundFile(fromExecutableParent)) {
            return fromExecutableParent;
        }
    }

    return relativeSoundPath;
}

std::string shellQuote(const std::string& value)
{
    std::string quoted = "'";
    for (const char character : value) {
        if (character == '\'') {
            quoted += "'\\''";
        } else {
            quoted += character;
        }
    }
    quoted += "'";
    return quoted;
}

}

namespace AudioAlert {

const std::string& reminderSoundPath()
{
    // 音频资源路径只解析一次，CLI 和 Qt GUI 调用时复用同一个结果。
    static const std::string path = findReminderSoundPath().string();
    return path;
}

bool canPlayReminderSound(std::string& message)
{
    // 播放前检查资源文件和 mpg123，失败时只返回错误信息，不抛异常。
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
    // ==================== 功能：播放提醒音频 ====================
    if (!canPlayReminderSound(message)) {
        return false;
    }
    const std::string command =
        "mpg123 " + shellQuote(reminderSoundPath()) + " >/dev/null 2>&1";
    if (std::system(command.c_str()) != 0) {
        message = "Failed to play reminder sound.";
        return false;
    }

    message.clear();
    return true;
}

void playReminderSoundAsync()
{
    // 使用原子标记避免多个提醒同时触发时叠加播放多个音频进程。
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
