#ifndef MYSCHEDULE_STORAGE_H
#define MYSCHEDULE_STORAGE_H

/*
 * 文件职责：
 * 声明项目的文件存储接口，集中管理 data 目录、用户文件和任务文件路径。
 *
 * 主要功能定位：
 * - 加载任务文件：搜索“功能：加载用户任务”
 * - 保存任务文件：搜索“功能：保存用户任务”
 */

#include "task.h"

#include <map>
#include <string>
#include <vector>

/**
 * @brief users.txt 中一条用户记录
 *
 * passwordHash 是加盐后的哈希结果，不保存明文密码。
 */
struct UserRecord {
    std::string salt;
    std::string passwordHash;
};

/**
 * @brief 单个用户任务文件加载后的结果
 *
 * tasks 保存解析出的任务列表，nextId 保存下一次新增任务应使用的 ID。
 */
struct TaskFileData {
    std::vector<Task> tasks;
    int nextId = 1;
};

/**
 * @brief 文件存储工具类
 *
 * 只负责路径定位、文本解析和原子写入；任务唯一性等业务规则由 TaskManager 处理。
 */
class Storage {
public:
    static bool ensureDataDir(std::string& errorMessage);
    static std::map<std::string, UserRecord> loadUsers(std::string& errorMessage);
    static bool saveUsers(const std::map<std::string, UserRecord>& users, std::string& errorMessage);
    static TaskFileData loadTasks(const std::string& username, std::string& errorMessage);
    static bool saveTasks(const std::string& username,
                          const std::vector<Task>& tasks,
                          int nextId,
                          std::string& errorMessage);

private:
    static std::string rootDir();
    static std::string dataDir();
    static std::string usersPath();
    static std::string tasksPath(const std::string& username);
};

#endif
