#include "storage.h"

#include <array>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <limits.h>
#include <sstream>
#include <system_error>
#include <unistd.h>

namespace fs = std::filesystem;

namespace {
constexpr const char* kTaskHeader =
    "id|name|start_time|priority|category|remind_time|reminded";

bool hasProjectMarkers(const fs::path& candidate) {
    std::error_code error;
    return fs::exists(candidate / "data", error) &&
           fs::exists(candidate / "CMakeLists.txt", error);
}

fs::path findProjectRootFrom(const fs::path& start) {
    fs::path current = start;
    for (int depth = 0; depth < 8; ++depth) {
        if (hasProjectMarkers(current)) {
            return current;
        }
        if (!current.has_parent_path() || current.parent_path() == current) {
            break;
        }
        current = current.parent_path();
    }
    return {};
}

fs::path executableDir() {
    std::array<char, PATH_MAX> buffer {};
    const ssize_t length = readlink("/proc/self/exe", buffer.data(), buffer.size() - 1);
    if (length <= 0) {
        return {};
    }
    buffer[static_cast<std::size_t>(length)] = '\0';
    return fs::path(buffer.data()).parent_path();
}
}

std::string Storage::rootDir() {
    const fs::path fromExecutable = findProjectRootFrom(executableDir());
    if (!fromExecutable.empty()) {
        return fromExecutable.string();
    }

    std::error_code error;
    const fs::path fromCurrentDirectory = findProjectRootFrom(fs::current_path(error));
    if (!fromCurrentDirectory.empty()) {
        return fromCurrentDirectory.string();
    }

    if (!error) {
        return fs::current_path(error).string();
    }
    return ".";
}

std::string Storage::dataDir() {
    const char* envDataDir = std::getenv("MYSCHEDULE_DATA_DIR");
    if (envDataDir != nullptr && envDataDir[0] != '\0') {
        std::error_code error;
        return fs::absolute(fs::path(envDataDir), error).string();
    }
    return (fs::path(rootDir()) / "data").string();
}

std::string Storage::usersPath() {
    return dataDir() + "/users.txt";
}

std::string Storage::tasksPath(const std::string& username) {
    return dataDir() + "/tasks_" + username + ".txt";
}

bool Storage::ensureDataDir(std::string& errorMessage) {
    try {
        fs::create_directories(dataDir());
        return true;
    } catch (const std::exception& error) {
        errorMessage = std::string("Error: failed to create data directory. ") + error.what();
        return false;
    }
}

std::map<std::string, UserRecord> Storage::loadUsers(std::string& errorMessage) {
    std::map<std::string, UserRecord> users;
    if (!ensureDataDir(errorMessage)) {
        return users;
    }

    std::ifstream input(usersPath());
    if (!input.is_open()) {
        return users;
    }

    std::string line;
    while (std::getline(input, line)) {
        if (line.empty()) {
            continue;
        }

        std::stringstream stream(line);
        std::string username;
        std::string salt;
        std::string hash;
        if (!std::getline(stream, username, ':') ||
            !std::getline(stream, salt, ':') ||
            !std::getline(stream, hash)) {
            errorMessage = "Error: invalid users.txt format.";
            users.clear();
            return users;
        }
        users[username] = UserRecord{salt, hash};
    }
    if (input.bad()) {
        errorMessage = "Error: failed while reading users.txt.";
        users.clear();
    }
    return users;
}

bool Storage::saveUsers(const std::map<std::string, UserRecord>& users, std::string& errorMessage) {
    if (!ensureDataDir(errorMessage)) {
        return false;
    }

    std::ofstream output(usersPath(), std::ios::trunc);
    if (!output.is_open()) {
        errorMessage = "Error: failed to open users.txt for writing.";
        return false;
    }

    for (const auto& [username, record] : users) {
        output << username << ':' << record.salt << ':' << record.passwordHash << '\n';
    }
    if (!output.good()) {
        errorMessage = "Error: failed while writing users.txt.";
        return false;
    }
    return true;
}

std::vector<Task> Storage::loadTasks(const std::string& username, std::string& errorMessage) {
    std::vector<Task> tasks;
    if (!ensureDataDir(errorMessage)) {
        return tasks;
    }

    std::ifstream input(tasksPath(username));
    if (!input.is_open()) {
        return tasks;
    }

    std::string line;
    while (std::getline(input, line)) {
        if (line.empty() || line == kTaskHeader) {
            continue;
        }

        std::string parseError;
        const auto parsed = deserializeTask(line, parseError);
        if (!parsed.has_value()) {
            errorMessage = parseError;
            return {};
        }
        tasks.push_back(*parsed);
    }
    if (input.bad()) {
        errorMessage = "Error: failed while reading task storage file.";
        return {};
    }
    return tasks;
}

bool Storage::saveTasks(const std::string& username,
                        const std::vector<Task>& tasks,
                        std::string& errorMessage) {
    if (!ensureDataDir(errorMessage)) {
        return false;
    }

    std::ofstream output(tasksPath(username), std::ios::trunc);
    if (!output.is_open()) {
        errorMessage = "Error: failed to open task storage file for writing.";
        return false;
    }

    output << kTaskHeader << '\n';
    for (const auto& task : tasks) {
        output << serializeTask(task) << '\n';
    }
    if (!output.good()) {
        errorMessage = "Error: failed while writing task storage file.";
        return false;
    }
    return true;
}

std::string Storage::taskFilePathForUser(const std::string& username) {
    return tasksPath(username);
}
