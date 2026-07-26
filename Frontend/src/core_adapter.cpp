#include "core_adapter.h"

/*
 * 文件职责：
 * 实现 Qt GUI 到核心层的调用适配，包括 QString/std::string 转换、
 * GUI task/Core Task 转换，以及用户和任务接口的转发。
 *
 * 主要功能定位：
 * - 功能：Qt 核心层适配
 * - Qt 登录/注册/任务管理最终从这里进入核心层
 */

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
    // Qt 输入进入核心层前统一 trim，避免用户名和文本字段带首尾空白。
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
    // 核心层时间是 "YYYY-MM-DD HH:MM"，Qt 任务模型用 epoch seconds 展示。
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
    // GUI 的 due/remind time 转回核心层固定字符串格式，便于复用核心校验和存储。
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

// ==================== 功能：Qt 核心层适配 ====================
CoreAdapter::LoginResult CoreAdapter::login(const QString& username, const QString& password)
{
    // 登录转发到 UserManager；这里把核心层英文错误映射为 GUI 需要的中文状态。
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
    // 注册转发到核心层，密码哈希和用户文件保存均由 UserManager/Storage 完成。
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
    // 加载任务：TaskManager 读取核心任务，再逐条转换成 GUI task 供表格展示。
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
    // 添加任务：GUI task -> Core Task -> TaskManager::addTask。
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
    // 修改任务：保留界面选中的真实 taskId，由核心层检查冲突并保存。
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
    // 删除任务：界面只传 ID，核心层负责查找、删除、保存和失败回滚。
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
    // Qt 提醒轮询复用核心层 collectDueReminders，再把到期任务转为 GUI task。
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
