#include "task_manager.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <system_error>
#include <vector>

namespace fs = std::filesystem;

namespace {
int fail(const std::string& message) {
    std::cerr << message << '\n';
    return 1;
}

Task makeTask(const std::string& name,
              const std::string& startTime,
              const std::string& priority,
              const std::string& category,
              const std::string& remindTime) {
    Task task;
    task.name = name;
    task.startTime = startTime;
    task.priority = priority;
    task.category = category;
    task.remindTime = remindTime;
    return task;
}

const Task* findTask(const std::vector<Task>& tasks, int id) {
    for (const auto& task : tasks) {
        if (task.id == id) {
            return &task;
        }
    }
    return nullptr;
}

bool writeTextFile(const fs::path& path, const std::string& contents) {
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    output << contents;
    output.close();
    return !output.fail();
}

std::string readTextFile(const fs::path& path) {
    std::ifstream input(path, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(input),
                       std::istreambuf_iterator<char>());
}
}

int main() {
    const fs::path testDataDir =
        fs::temp_directory_path() / "myschedule_task_manager_update_test";
    std::error_code error;
    fs::remove_all(testDataDir, error);
    fs::create_directories(testDataDir, error);
    if (error) {
        return fail("Failed to create isolated test data directory: " + error.message());
    }
    setenv("MYSCHEDULE_DATA_DIR", testDataDir.c_str(), 1);

    const std::string username = "core_update_test";
    std::string message;

    TaskManager manager(username);
    if (!manager.load(message)) {
        return fail("Failed to load empty task list: " + message);
    }

    Task first = makeTask(
        "first",
        "2099-01-01 10:00",
        "medium",
        "life",
        "2000-01-01 00:00");
    if (!manager.addTask(first, message)) {
        return fail("Failed to add first task: " + message);
    }
    const int firstId = manager.getAllTasks().front().id;

    Task second = makeTask(
        "second",
        "2099-01-02 10:00",
        "low",
        "entertainment",
        "2099-01-02 09:30");
    if (!manager.addTask(second, message)) {
        return fail("Failed to add second task: " + message);
    }
    const int secondId = manager.getAllTasks().back().id;

    std::vector<Task> reminders;
    if (!manager.collectDueReminders(reminders, message) || reminders.size() != 1U) {
        return fail("Failed to mark the first task as reminded.");
    }

    const auto tasksAfterReminder = manager.getAllTasks();
    const Task* remindedTask = findTask(tasksAfterReminder, firstId);
    if (remindedTask == nullptr) {
        return fail("First task was not found after reminder collection.");
    }
    Task priorityCategoryUpdate = *remindedTask;
    priorityCategoryUpdate.name = "renamed";
    priorityCategoryUpdate.priority = "high";
    priorityCategoryUpdate.category = "study";
    if (!manager.updateTask(firstId, priorityCategoryUpdate, message)) {
        return fail("Failed to update name, priority, and category: " + message);
    }

    TaskManager reloadedAfterFieldUpdate(username);
    if (!reloadedAfterFieldUpdate.load(message)) {
        return fail("Failed to reload after field update: " + message);
    }
    const auto tasksAfterFieldUpdate = reloadedAfterFieldUpdate.getAllTasks();
    const Task* persisted = findTask(tasksAfterFieldUpdate, firstId);
    if (persisted == nullptr) {
        return fail("Updated task was not persisted.");
    }
    if (persisted->name != "renamed" ||
        persisted->priority != "high" ||
        persisted->category != "study") {
        return fail("Updated name, priority, or category was not persisted.");
    }
    if (!persisted->reminded) {
        return fail("Updating only name/priority/category should keep reminded=true.");
    }

    Task timeUpdate = *persisted;
    timeUpdate.startTime = "2099-01-03 10:00";
    timeUpdate.remindTime = "2099-01-03 09:30";
    if (!reloadedAfterFieldUpdate.updateTask(firstId, timeUpdate, message)) {
        return fail("Failed to update start and remind times: " + message);
    }

    TaskManager reloadedAfterTimeUpdate(username);
    if (!reloadedAfterTimeUpdate.load(message)) {
        return fail("Failed to reload after time update: " + message);
    }
    const auto tasksAfterTimeUpdate = reloadedAfterTimeUpdate.getAllTasks();
    persisted = findTask(tasksAfterTimeUpdate, firstId);
    if (persisted == nullptr || persisted->reminded) {
        return fail("Changing start/remind time should reset reminded=false.");
    }

    Task conflictingUpdate = *persisted;
    conflictingUpdate.startTime = "2099-01-02 10:00";
    conflictingUpdate.remindTime = "2099-01-02 09:30";
    if (reloadedAfterTimeUpdate.updateTask(firstId, conflictingUpdate, message)) {
        return fail("Updating to a conflicting start time unexpectedly succeeded.");
    }

    if (!reloadedAfterTimeUpdate.deleteTaskById(secondId, message)) {
        return fail("Failed to delete the current maximum task id: " + message);
    }

    TaskManager reloadedAfterMaxDelete(username);
    if (!reloadedAfterMaxDelete.load(message)) {
        return fail("Failed to reload after deleting the maximum id: " + message);
    }
    Task third = makeTask(
        "third",
        "2099-01-04 10:00",
        "medium",
        "life",
        "2099-01-04 09:30");
    if (!reloadedAfterMaxDelete.addTask(third, message)) {
        return fail("Failed to add after deleting the maximum id: " + message);
    }
    const auto tasksAfterIdReuseCheck = reloadedAfterMaxDelete.getAllTasks();
    const Task* addedAfterDelete = nullptr;
    for (const auto& task : tasksAfterIdReuseCheck) {
        if (task.name == "third") {
            addedAfterDelete = &task;
            break;
        }
    }
    if (addedAfterDelete == nullptr || addedAfterDelete->id <= secondId) {
        return fail("A deleted maximum task id was reused.");
    }

    std::string parseError;
    if (deserializeTask(
            "12abc|bad|2099-01-01 10:00|medium|life|2099-01-01 09:00|0",
            parseError)
            .has_value()) {
        return fail("A stored task id with trailing characters was accepted.");
    }
    if (deserializeTask(
            "12|bad|2099-01-01 10:00|medium|life|2099-01-01 09:00|garbage",
            parseError)
            .has_value()) {
        return fail("An invalid stored reminded value was accepted.");
    }

    const fs::path corruptDataPath = testDataDir / "corrupt-data";
    fs::create_directories(corruptDataPath, error);
    if (error) {
        return fail("Failed to create corrupt-data test directory: " + error.message());
    }
    setenv("MYSCHEDULE_DATA_DIR", corruptDataPath.c_str(), 1);
    const fs::path corruptTaskPath = corruptDataPath / "tasks_corrupt.txt";
    const std::string header =
        "id|name|start_time|priority|category|remind_time|reminded\n";
    const auto expectCorruptLoadRejected = [&](const std::string& contents,
                                                const std::string& expectedError) {
        if (!writeTextFile(corruptTaskPath, contents)) {
            return false;
        }
        TaskManager corruptManager("corrupt");
        std::string loadMessage;
        if (corruptManager.load(loadMessage)) {
            return false;
        }
        return loadMessage.find(expectedError) != std::string::npos &&
               readTextFile(corruptTaskPath) == contents;
    };

    if (!expectCorruptLoadRejected(
            header + "next_id|2\n1|missing-fields\n",
            "invalid task record format")) {
        return fail("A task record with missing fields was accepted or overwritten.");
    }
    if (!expectCorruptLoadRejected(
            header + "next_id|2\n12abc|bad|2099-01-01 10:00|medium|life|2099-01-01 09:00|0\n",
            "invalid task id")) {
        return fail("A task record with an invalid id was accepted or overwritten.");
    }
    if (!expectCorruptLoadRejected(
            header + "next_id|3\n"
                     "1|first|2099-01-01 10:00|medium|life|2099-01-01 09:00|0\n"
                     "1|second|2099-01-02 10:00|medium|life|2099-01-02 09:00|0\n",
            "duplicate task id")) {
        return fail("Duplicate stored task ids were accepted or overwritten.");
    }
    if (!expectCorruptLoadRejected(
            header + "next_id|2\n1|bad-time|2099-13-01 10:00|medium|life|2099-01-01 09:00|0\n",
            "invalid time format")) {
        return fail("An invalid stored task time was accepted or overwritten.");
    }
    if (!expectCorruptLoadRejected(
            header + "next_id|2\n1|bad-priority|2099-01-01 10:00|urgent|life|2099-01-01 09:00|0\n",
            "invalid priority")) {
        return fail("An invalid stored priority was accepted or overwritten.");
    }
    if (!expectCorruptLoadRejected(
            header + "next_id|2\n1|bad-category|2099-01-01 10:00|medium|work|2099-01-01 09:00|0\n",
            "invalid category")) {
        return fail("An invalid stored category was accepted or overwritten.");
    }
    if (!expectCorruptLoadRejected(
            header + "next_id|2\n1|bad-reminded|2099-01-01 10:00|medium|life|2099-01-01 09:00|2\n",
            "invalid reminded value")) {
        return fail("An invalid stored reminded state was accepted or overwritten.");
    }
    if (!expectCorruptLoadRejected(
            header + "next_id|broken\n",
            "invalid next_id metadata")) {
        return fail("Invalid next_id metadata was accepted or overwritten.");
    }
    if (!expectCorruptLoadRejected(
            header + "next_id|3\n"
                     "1|valid-first|2099-01-01 10:00|medium|life|2099-01-01 09:00|0\n"
                     "2|partially-written|2099-01-02 10:00|medium",
            "invalid task record format")) {
        return fail("A partially written task file was accepted or overwritten.");
    }

    const fs::path rollbackDataPath = testDataDir / "rollback-data";
    setenv("MYSCHEDULE_DATA_DIR", rollbackDataPath.c_str(), 1);
    TaskManager saveFailureManager("save_failure");
    if (!saveFailureManager.load(message)) {
        return fail("Failed to prepare save-failure test: " + message);
    }
    Task saveFailureTask = makeTask(
        "must remain in memory",
        "2099-02-01 10:00",
        "medium",
        "life",
        "2000-01-01 00:00");
    if (!saveFailureManager.addTask(saveFailureTask, message)) {
        return fail("Failed to add save-failure test task: " + message);
    }
    const int saveFailureId = saveFailureManager.getAllTasks().front().id;
    const int nextIdBeforeFailures = saveFailureManager.generateNextId();
    const fs::path taskFilePath = rollbackDataPath / "tasks_save_failure.txt";
    const std::string originalFileContents = readTextFile(taskFilePath);
    const fs::path temporaryPath = taskFilePath.string() + ".tmp";
    fs::create_directories(temporaryPath, error);
    if (error || !writeTextFile(temporaryPath / "blocker", "not empty\n")) {
        return fail("Failed to block temporary storage file creation.");
    }

    Task failedAdd = makeTask(
        "must not be added",
        "2099-02-02 10:00",
        "high",
        "study",
        "2099-02-02 09:00");
    if (saveFailureManager.addTask(failedAdd, message)) {
        return fail("Add unexpectedly succeeded when temporary storage was unavailable.");
    }
    if (saveFailureManager.getAllTasks().size() != 1U ||
        saveFailureManager.generateNextId() != nextIdBeforeFailures ||
        readTextFile(taskFilePath) != originalFileContents) {
        return fail("Failed add was not fully rolled back or changed the original file.");
    }

    Task failedUpdate = saveFailureManager.getAllTasks().front();
    failedUpdate.name = "must not replace original";
    failedUpdate.startTime = "2099-02-03 10:00";
    failedUpdate.remindTime = "2099-02-03 09:00";
    failedUpdate.priority = "low";
    failedUpdate.category = "entertainment";
    if (saveFailureManager.updateTask(saveFailureId, failedUpdate, message)) {
        return fail("Update unexpectedly succeeded when temporary storage was unavailable.");
    }
    const auto tasksAfterFailedUpdate = saveFailureManager.getAllTasks();
    const Task* taskAfterFailedUpdate = findTask(tasksAfterFailedUpdate, saveFailureId);
    if (taskAfterFailedUpdate == nullptr ||
        taskAfterFailedUpdate->name != "must remain in memory" ||
        taskAfterFailedUpdate->startTime != "2099-02-01 10:00" ||
        taskAfterFailedUpdate->priority != "medium" ||
        taskAfterFailedUpdate->category != "life" ||
        readTextFile(taskFilePath) != originalFileContents) {
        return fail("Failed update was not fully rolled back or changed the original file.");
    }

    if (saveFailureManager.deleteTaskById(saveFailureId, message)) {
        return fail("Delete unexpectedly succeeded when storage was unavailable.");
    }
    const auto tasksAfterFailedDelete = saveFailureManager.getAllTasks();
    if (findTask(tasksAfterFailedDelete, saveFailureId) == nullptr) {
        return fail("Failed delete was not rolled back in memory.");
    }
    if (readTextFile(taskFilePath) != originalFileContents) {
        return fail("Failed delete changed or truncated the original file.");
    }

    reminders.clear();
    if (saveFailureManager.collectDueReminders(reminders, message)) {
        return fail("Reminder persistence unexpectedly succeeded with unavailable storage.");
    }
    const auto tasksAfterFailedReminder = saveFailureManager.getAllTasks();
    const Task* taskAfterFailedReminder = findTask(tasksAfterFailedReminder, saveFailureId);
    if (!reminders.empty() || taskAfterFailedReminder == nullptr || taskAfterFailedReminder->reminded) {
        return fail("Failed reminder persistence was not rolled back in memory.");
    }
    if (readTextFile(taskFilePath) != originalFileContents) {
        return fail("Failed reminder persistence changed the original file.");
    }

    fs::remove_all(temporaryPath, error);
    if (error) {
        return fail("Failed to unblock temporary storage path: " + error.message());
    }
    if (!saveFailureManager.addTask(failedAdd, message)) {
        return fail("Add did not recover after storage became writable: " + message);
    }
    const Task* addedAfterFailure = nullptr;
    for (const auto& task : saveFailureManager.getAllTasks()) {
        if (task.name == "must not be added") {
            addedAfterFailure = &task;
            break;
        }
    }
    if (addedAfterFailure == nullptr || addedAfterFailure->id != nextIdBeforeFailures) {
        return fail("A failed add incorrectly consumed the next task id.");
    }

    fs::remove_all(testDataDir, error);
    if (error) {
        return fail("Failed to remove isolated test data directory: " + error.message());
    }

    std::cout << "TaskManager update test passed.\n";
    return 0;
}
