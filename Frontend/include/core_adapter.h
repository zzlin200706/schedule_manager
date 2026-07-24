#ifndef CORE_ADAPTER_H
#define CORE_ADAPTER_H

#include <QString>
#include <vector>

#include "task.h"

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
