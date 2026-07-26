#ifndef CORE_ADAPTER_H
#define CORE_ADAPTER_H

/*
 * 文件职责：
 * 声明 Qt GUI 与核心层之间的适配接口。Qt 界面使用 QString、time_t 和
 * Frontend::task，核心层使用 std::string 和 Task，本类负责两边转换。
 *
 * 主要功能定位：
 * - 功能：Qt 核心层适配
 */

#include <QString>
#include <vector>

#include "task.h"

/**
 * @brief Qt 与核心层的适配器
 *
 * 适配层只负责类型转换和调用转发，具体业务规则由核心层实现。
 * 例如任务唯一性、密码哈希、文件保存失败回滚都不在这里实现。
 */
class CoreAdapter {
public:
    enum class LoginStatus {
        Ok,
        UserNotFound,
        WrongPassword,
        Error
    };

    struct LoginResult {
        LoginStatus status = LoginStatus::Error;
        QString message;
    };

    LoginResult login(const QString& username, const QString& password);
    bool registerUser(const QString& username, const QString& password, QString& message);
    bool userExists(const QString& username);
    bool loadTasks(const QString& username, std::vector<task>& tasks, QString& message);
    bool addTask(const QString& username, const task& guiTask, QString& message);
    bool updateTask(const QString& username, int taskId, const task& guiTask, QString& message);
    bool deleteTask(const QString& username, int taskId, QString& message);
    bool collectDueReminders(const QString& username, std::vector<task>& reminders, QString& message);
};

#endif
