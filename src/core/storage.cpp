#include "storage.h"

/*
 * 文件职责：
 * 实现用户账号文件和任务数据文件的加载与保存。用户信息保存在
 * users.txt，任务保存在 tasks_<username>.txt。
 *
 * 主要功能定位：
 * - 功能：加载用户任务
 * - 功能：保存用户任务
 * - 用户账号文件读写由 UserManager 间接调用
 */

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
constexpr const char* kNextIdPrefix = "next_id|";

bool hasProjectMarkers(const fs::path& candidate) {
    std::error_code error;
    return fs::exists(candidate / "data", error) &&
           fs::exists(candidate / "CMakeLists.txt", error);
}

fs::path findProjectRootFrom(const fs::path& start) {
    // 从可执行文件目录或当前工作目录向上查找，兼容 CLI/Qt 从不同目录启动。
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

bool parsePositiveInteger(const std::string& value, int& result) {
    if (value.empty()) {
        return false;
    }
    for (const unsigned char ch : value) {
        if (ch < '0' || ch > '9') {
            return false;
        }
    }

    try {
        std::size_t consumed = 0;
        const int parsed = std::stoi(value, &consumed, 10);
        if (consumed != value.size() || parsed <= 0) {
            return false;
        }
        result = parsed;
        return true;
    } catch (...) {
        return false;
    }
}

bool isValidStoredUsername(const std::string& username) {
    if (username.empty()) {
        return false;
    }
    for (const unsigned char ch : username) {
        const bool allowed =
            (ch >= 'a' && ch <= 'z') ||
            (ch >= 'A' && ch <= 'Z') ||
            (ch >= '0' && ch <= '9') ||
            ch == '_' || ch == '-';
        if (!allowed) {
            return false;
        }
    }
    return true;
}

bool isHex64(const std::string& value) {
    if (value.size() != 16U) {
        return false;
    }
    for (const unsigned char ch : value) {
        const bool hexadecimal =
            (ch >= '0' && ch <= '9') ||
            (ch >= 'a' && ch <= 'f') ||
            (ch >= 'A' && ch <= 'F');
        if (!hexadecimal) {
            return false;
        }
    }
    return true;
}

bool writeTextFileAtomically(const fs::path& path,
                             const std::string& contents,
                             std::string& errorMessage) {
    // 先写 .tmp 再 rename，避免写入中断时破坏原任务/用户文件。
    fs::path temporaryPath = path;
    temporaryPath += ".tmp";

    std::error_code error;
    fs::remove(temporaryPath, error);
    error.clear();

    std::ofstream output(temporaryPath, std::ios::binary | std::ios::trunc);
    if (!output.is_open()) {
        errorMessage = "Error: failed to open temporary storage file for writing.";
        return false;
    }

    output << contents;
    output.flush();
    if (!output.good()) {
        output.close();
        fs::remove(temporaryPath, error);
        errorMessage = "Error: failed while writing temporary storage file.";
        return false;
    }

    output.close();
    if (output.fail()) {
        fs::remove(temporaryPath, error);
        errorMessage = "Error: failed to close temporary storage file.";
        return false;
    }

    fs::rename(temporaryPath, path, error);
    if (error) {
        fs::remove(temporaryPath, error);
        errorMessage = "Error: failed to replace storage file.";
        return false;
    }
    return true;
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
    } catch (const std::exception& ex) {
        errorMessage = std::string("Error: failed to create data directory. ") + ex.what();
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
        if (!writeTextFileAtomically(usersPath(), "", errorMessage)) {
            errorMessage = "Error: failed to create user storage file. " + errorMessage;
        }
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
        // users.txt 每行格式：username:salt:passwordHash。
        if (!std::getline(stream, username, ':') ||
            !std::getline(stream, salt, ':') ||
            !std::getline(stream, hash) ||
            !isValidStoredUsername(username) || !isHex64(salt) || !isHex64(hash) ||
            hash.find(':') != std::string::npos ||
            users.find(username) != users.end()) {
            errorMessage = "Error: invalid users.txt format.";
            users.clear();
            return users;
        }
        users[username] = UserRecord{salt, hash};
    }
    return users;
}

bool Storage::saveUsers(const std::map<std::string, UserRecord>& users, std::string& errorMessage) {
    if (!ensureDataDir(errorMessage)) {
        return false;
    }

    std::ostringstream contents;
    for (const auto& [username, record] : users) {
        contents << username << ':' << record.salt << ':' << record.passwordHash << '\n';
    }
    return writeTextFileAtomically(usersPath(), contents.str(), errorMessage);
}

// ==================== 功能：加载用户任务 ====================
TaskFileData Storage::loadTasks(const std::string& username, std::string& errorMessage) {
    TaskFileData data;
    if (!ensureDataDir(errorMessage)) {
        return data;
    }

    const std::string path = tasksPath(username);
    std::ifstream input(path);
    if (!input.is_open()) {
        // 首次登录该用户时创建空任务文件，并写入表头与 next_id 元数据。
        const std::string initialContents =
            std::string(kTaskHeader) + "\n" + kNextIdPrefix + "1\n";
        if (!writeTextFileAtomically(path, initialContents, errorMessage)) {
            errorMessage = "Error: failed to create task storage file for user. " + errorMessage;
        }
        return data;
    }

    std::string line;
    bool foundNextId = false;
    while (std::getline(input, line)) {
        if (line.empty()) {
            continue;
        }
        if (line == kTaskHeader) {
            continue;
        }
        if (line.rfind(kNextIdPrefix, 0) == 0) {
            // next_id 用于恢复下一次新增任务的唯一 ID。
            int parsedNextId = 0;
            if (foundNextId ||
                !parsePositiveInteger(line.substr(std::char_traits<char>::length(kNextIdPrefix)),
                                      parsedNextId)) {
                errorMessage = "Error: invalid next_id metadata in task storage.";
                return {};
            }
            data.nextId = parsedNextId;
            foundNextId = true;
            continue;
        }
        std::string parseError;
        // 普通任务行交给 task.cpp 解析，保持字段校验逻辑集中。
        const auto parsed = deserializeTask(line, parseError);
        if (!parsed.has_value()) {
            errorMessage = parseError;
            return {};
        }
        data.tasks.push_back(*parsed);
    }
    if (input.bad()) {
        errorMessage = "Error: failed while reading task storage file.";
        return {};
    }
    return data;
}

// ==================== 功能：保存用户任务 ====================
bool Storage::saveTasks(const std::string& username,
                        const std::vector<Task>& tasks,
                        int nextId,
                        std::string& errorMessage) {
    if (!ensureDataDir(errorMessage)) {
        return false;
    }
    if (nextId <= 0) {
        errorMessage = "Error: next task id must be positive.";
        return false;
    }

    std::ostringstream contents;
    // 任务文件格式：表头 + next_id 元数据 + 多行序列化任务记录。
    contents << kTaskHeader << '\n';
    contents << kNextIdPrefix << nextId << '\n';
    for (const auto& task : tasks) {
        contents << serializeTask(task) << '\n';
    }
    return writeTextFileAtomically(tasksPath(username), contents.str(), errorMessage);
}
