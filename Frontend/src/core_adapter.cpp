#include "core_adapter.h"

#include "../../src/core/task_manager.h"
#include "../../src/core/time_utils.h"
#include "../../src/core/user.h"

#include <QJsonObject>

#include <ctime>
#include <iomanip>
#include <sstream>

namespace {
std::string toStdString(const QString& value)
{
    return value.trimmed().toStdString();
}

QString toQString(const std::string& value)
{
    return QString::fromStdString(value);
}

bool toEpochSeconds(const std::string& value, time_t& result)
{
    const auto parsed = parseDateTime(value);
    if (!parsed.has_value()) {
        return false;
    }
    result = toTimeT(*parsed);
    return true;
}

QString toCoreDateTimeString(time_t value, QString& errorMessage)
{
    std::tm* localTime = std::localtime(&value);
    if (localTime == nullptr) {
        errorMessage = QStringLiteral("时间无法转换");
        return {};
    }

    std::ostringstream output;
    output << std::put_time(localTime, "%Y-%m-%d %H:%M");
    return QString::fromStdString(output.str());
}

task coreTaskToGuiTask(const Task& coreTask, QString& errorMessage)
{
    time_t dueTime = 0;
    if (!toEpochSeconds(coreTask.startTime, dueTime)) {
        errorMessage = QStringLiteral("任务时间格式无法转换: %1")
                           .arg(QString::fromStdString(coreTask.startTime));
        return {};
    }

    time_t remindTime = 0;
    if (!coreTask.remindTime.empty() && !toEpochSeconds(coreTask.remindTime, remindTime)) {
        errorMessage = QStringLiteral("提醒时间格式无法转换: %1")
                           .arg(QString::fromStdString(coreTask.remindTime));
        return {};
    }

    QJsonObject obj;
    obj["task_name"] = QString::fromStdString(coreTask.name);
    obj["task_priority"] = QString::fromStdString(coreTask.priority);
    obj["task_category"] = QString::fromStdString(coreTask.category);
    obj["task_due_time"] = static_cast<double>(dueTime);
    obj["task_remind_time"] = static_cast<double>(remindTime);
    obj["task_reminded"] = coreTask.reminded;
    obj["task_id"] = coreTask.id;
    return task_from_json(obj);
}

bool guiTaskToCoreTask(const task& guiTask, Task& coreTask, QString& errorMessage)
{
    const QString startTime = toCoreDateTimeString(guiTask.get_task_due_time(), errorMessage);
    if (!errorMessage.isEmpty()) {
        return false;
    }

    const time_t guiRemindTime = guiTask.get_task_remind_time();
    QString remindTime = startTime;
    if (guiRemindTime > 0) {
        remindTime = toCoreDateTimeString(guiRemindTime, errorMessage);
        if (!errorMessage.isEmpty()) {
            return false;
        }
    }

    coreTask = Task{};
    coreTask.name = guiTask.get_task_name();
    coreTask.startTime = startTime.toStdString();
    coreTask.priority = guiTask.get_task_priority();
    coreTask.category = guiTask.get_task_category();
    coreTask.remindTime = remindTime.toStdString();
    coreTask.reminded = guiTask.get_task_reminded();
    return true;
}
}

CoreAdapter::LoginResult CoreAdapter::login(const QString& username, const QString& password)
{
    const std::string user = toStdString(username);
    const std::string pass = password.toStdString();

    UserManager userManager;
    std::string message;
    if (!userManager.loadSucceeded(message)) {
        return {LoginStatus::Error, toQString(message)};
    }
    if (!userManager.userExists(user)) {
        return {LoginStatus::UserNotFound, QStringLiteral("用户未注册")};
    }

    if (userManager.login(user, pass, message)) {
        return {LoginStatus::Ok, QStringLiteral("登录成功")};
    }

    if (message == "Error: username or password is incorrect.") {
        return {LoginStatus::WrongPassword, QStringLiteral("密码错误")};
    }

    return {LoginStatus::Error, toQString(message)};
}

bool CoreAdapter::registerUser(const QString& username, const QString& password, QString& message)
{
    UserManager userManager;
    std::string coreMessage;
    const bool ok = userManager.registerUser(toStdString(username), password.toStdString(), coreMessage);
    message = ok ? QStringLiteral("注册成功") : toQString(coreMessage);
    return ok;
}

bool CoreAdapter::userExists(const QString& username)
{
    UserManager userManager;
    return userManager.userExists(toStdString(username));
}

bool CoreAdapter::loadTasks(const QString& username, std::vector<task>& tasks, QString& message)
{
    TaskManager taskManager(toStdString(username));
    std::string coreMessage;
    if (!taskManager.load(coreMessage)) {
        tasks.clear();
        message = toQString(coreMessage);
        return false;
    }

    std::vector<task> loadedTasks;
    for (const auto& coreTask : taskManager.getAllTasks()) {
        QString conversionError;
        task guiTask = coreTaskToGuiTask(coreTask, conversionError);
        if (!conversionError.isEmpty()) {
            tasks.clear();
            message = conversionError;
            return false;
        }
        loadedTasks.push_back(guiTask);
    }

    tasks = std::move(loadedTasks);
    message = QStringLiteral("任务加载成功");
    return true;
}

bool CoreAdapter::addTask(const QString& username, const task& guiTask, QString& message)
{
    TaskManager taskManager(toStdString(username));
    std::string coreMessage;
    if (!taskManager.load(coreMessage)) {
        message = toQString(coreMessage);
        return false;
    }

    Task coreTask;
    QString conversionError;
    if (!guiTaskToCoreTask(guiTask, coreTask, conversionError)) {
        message = conversionError;
        return false;
    }

    if (!taskManager.addTask(coreTask, coreMessage)) {
        message = toQString(coreMessage);
        return false;
    }

    message = QStringLiteral("任务添加成功");
    return true;
}

bool CoreAdapter::updateTask(const QString& username, int taskId, const task& guiTask, QString& message)
{
    TaskManager taskManager(toStdString(username));
    std::string coreMessage;
    if (!taskManager.load(coreMessage)) {
        message = toQString(coreMessage);
        return false;
    }

    Task coreTask;
    QString conversionError;
    if (!guiTaskToCoreTask(guiTask, coreTask, conversionError)) {
        message = conversionError;
        return false;
    }
    coreTask.id = taskId;

    if (!taskManager.updateTask(taskId, coreTask, coreMessage)) {
        message = toQString(coreMessage);
        return false;
    }

    message = QStringLiteral("任务已更新");
    return true;
}

bool CoreAdapter::deleteTask(const QString& username, int taskId, QString& message)
{
    TaskManager taskManager(toStdString(username));
    std::string coreMessage;
    if (!taskManager.load(coreMessage)) {
        message = toQString(coreMessage);
        return false;
    }

    if (!taskManager.deleteTaskById(taskId, coreMessage)) {
        message = toQString(coreMessage);
        return false;
    }

    message = QStringLiteral("任务已删除");
    return true;
}

bool CoreAdapter::collectDueReminders(const QString& username, std::vector<task>& reminders, QString& message)
{
    TaskManager taskManager(toStdString(username));
    std::string coreMessage;
    if (!taskManager.load(coreMessage)) {
        reminders.clear();
        message = toQString(coreMessage);
        return false;
    }

    std::vector<Task> coreReminders;
    if (!taskManager.collectDueReminders(coreReminders, coreMessage)) {
        reminders.clear();
        message = toQString(coreMessage);
        return false;
    }

    std::vector<task> convertedReminders;
    convertedReminders.reserve(coreReminders.size());
    for (const auto& coreTask : coreReminders) {
        QString conversionError;
        task guiTask = coreTaskToGuiTask(coreTask, conversionError);
        if (!conversionError.isEmpty()) {
            reminders.clear();
            message = conversionError;
            return false;
        }
        convertedReminders.push_back(guiTask);
    }

    reminders = std::move(convertedReminders);
    message.clear();
    return true;
}
