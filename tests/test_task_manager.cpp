#include "task_manager.h"
#include "time_utils.h"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <system_error>
#include <unistd.h>
#include <vector>

namespace fs = std::filesystem;

namespace {
bool expect(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "Test failure: " << message << '\n';
        return false;
    }
    return true;
}

Task makeTask(const std::string& name,
              const std::string& startTime,
              const std::string& priority = "medium",
              const std::string& category = "life",
              const std::string& remindTime = "") {
    Task task;
    task.name = name;
    task.startTime = startTime;
    task.priority = priority;
    task.category = category;
    task.remindTime = remindTime;
    return task;
}

const Task* findTask(const std::vector<Task>& tasks, const std::string& name) {
    for (const auto& task : tasks) {
        if (task.name == name) {
            return &task;
        }
    }
    return nullptr;
}
}

int main() {
    char directoryTemplate[] = "/tmp/myschedule-task-test-XXXXXX";
    char* directory = mkdtemp(directoryTemplate);
    if (directory == nullptr) {
        std::cerr << "Test failure: unable to create temporary data directory.\n";
        return 1;
    }
    const fs::path dataDirectory(directory);
    setenv("MYSCHEDULE_DATA_DIR", dataDirectory.c_str(), 1);

    bool passed = true;
    passed = expect(parseDateTime("2028-02-29 09:05").has_value(),
                    "a valid leap-day timestamp should parse") &&
             expect(!parseDateTime("2027-02-29 09:05").has_value(),
                    "an invalid leap-day timestamp should fail") &&
             expect(!parseDateTime("2028-02-29 9:05").has_value(),
                    "timestamps should use strict zero-padded fields") &&
             expect(!parseDateTime("2028-13-01 09:05").has_value(),
                    "an invalid month should fail") &&
             expect(isValidDayString("2028-02-29"), "a valid day should pass") &&
             expect(!isValidDayString("2028-02-30"), "an invalid day should fail") &&
             expect(isValidMonthString("2028-02"), "a valid month should pass") &&
             expect(!isValidMonthString("2028-2"), "a non-padded month should fail") &&
             passed;

    std::string message;
    TaskManager manager("alice");
    passed = expect(manager.load(message), "an empty task list should load") && passed;

    Task invalidDate = makeTask("invalid", "2027-02-29 10:00");
    passed = expect(!manager.addTask(invalidDate, message), "an invalid date should be rejected") &&
             expect(manager.getAllTasks().empty(), "a rejected task should not be added") &&
             passed;

    Task morning = makeTask("morning class", "2030-05-01 09:00", "", "", "");
    passed = expect(manager.addTask(morning, message), "a task using defaults should be added") && passed;
    const auto tasksAfterMorning = manager.getAllTasks();
    const Task* storedMorning = findTask(tasksAfterMorning, "morning class");
    passed = expect(storedMorning != nullptr, "the defaulted task should be present") && passed;
    if (storedMorning != nullptr) {
        passed = expect(storedMorning->priority == "medium", "default priority should be medium") &&
                 expect(storedMorning->category == "life", "default category should be life") &&
                 expect(storedMorning->remindTime == storedMorning->startTime,
                        "default remind time should equal start time") &&
                 passed;
    }

    Task earlier = makeTask(
        "prepare notes", "2030-04-30 20:00", "high", "study", "2030-04-30 19:30");
    Task evening = makeTask(
        "watch film", "2030-05-01 20:00", "low", "entertainment", "2030-05-01 19:45");
    passed = expect(manager.addTask(earlier, message), "an earlier task should be added") &&
             expect(manager.addTask(evening, message), "an evening task should be added") &&
             passed;

    Task conflictingStart = makeTask("different name", "2030-05-01 09:00");
    passed = expect(!manager.addTask(conflictingStart, message),
                    "a duplicate start time should be rejected") && passed;

    Task lateReminder = makeTask(
        "late reminder", "2030-05-02 09:00", "medium", "life", "2030-05-02 09:01");
    passed = expect(!manager.addTask(lateReminder, message),
                    "a reminder later than the start should be rejected") && passed;

    const auto sorted = manager.getAllTasks();
    passed = expect(sorted.size() == 3U, "three valid tasks should remain") &&
             expect(sorted.size() == 3U && sorted[0].name == "prepare notes" &&
                        sorted[1].name == "morning class" && sorted[2].name == "watch film",
                    "all tasks should be sorted by real start time") && passed;

    const auto mayDay = manager.getTasksByDay("2030-05-01");
    const auto mayMonth = manager.getTasksByMonth("2030-05");
    passed = expect(mayDay.size() == 2U, "day query should return both May 1 tasks") &&
             expect(mayMonth.size() == 2U, "month query should return both May tasks") &&
             passed;

    const auto tasksBeforeUpdate = manager.getAllTasks();
    storedMorning = findTask(tasksBeforeUpdate, "morning class");
    int morningId = 0;
    if (storedMorning != nullptr) {
        morningId = storedMorning->id;
        Task updated = *storedMorning;
        updated.id = 9999;
        updated.name = "updated class";
        updated.startTime = "2030-05-02 08:30";
        updated.priority = "high";
        updated.category = "study";
        updated.remindTime = "2030-05-02 08:00";
        passed = expect(manager.updateTask(morningId, updated, message),
                        "all editable task fields should update") && passed;
    }

    const auto tasksAfterUpdate = manager.getAllTasks();
    const Task* updatedTask = findTask(tasksAfterUpdate, "updated class");
    passed = expect(updatedTask != nullptr, "the updated task should be present") && passed;
    if (updatedTask != nullptr) {
        passed = expect(updatedTask->id == morningId, "updating should preserve the task id") &&
                 expect(updatedTask->priority == "high", "priority should update") &&
                 expect(updatedTask->category == "study", "category should update") &&
                 expect(updatedTask->remindTime == "2030-05-02 08:00",
                        "remind time should update") &&
                 passed;
    }

    Task conflictingUpdate = updatedTask == nullptr ? Task{} : *updatedTask;
    conflictingUpdate.startTime = "2030-05-01 20:00";
    conflictingUpdate.remindTime = "2030-05-01 19:30";
    passed = expect(!manager.updateTask(morningId, conflictingUpdate, message),
                    "an update to an occupied start time should be rejected") && passed;

    TaskManager reloaded("alice");
    passed = expect(reloaded.load(message), "saved tasks should reload") && passed;
    const auto reloadedTasks = reloaded.getAllTasks();
    const Task* persisted = findTask(reloadedTasks, "updated class");
    passed = expect(persisted != nullptr && persisted->id == morningId,
                    "updated fields and id should persist") && passed;

    const Task* earlierTask = findTask(reloadedTasks, "prepare notes");
    if (earlierTask == nullptr) {
        passed = expect(false, "the task selected for deletion should exist") && passed;
    } else {
        const int deletedId = earlierTask->id;
        passed = expect(reloaded.deleteTaskById(deletedId, message),
                        "an existing task should be deleted") &&
                 expect(!reloaded.deleteTaskById(deletedId, message),
                        "deleting the same task twice should fail") &&
                 passed;
    }

    TaskManager afterDelete("alice");
    passed = expect(afterDelete.load(message), "tasks should reload after deletion") && passed;
    const auto tasksAfterDelete = afterDelete.getAllTasks();
    passed = expect(findTask(tasksAfterDelete, "prepare notes") == nullptr,
                    "deletion should persist") &&
             passed;

    std::string parseError;
    Task roundTrip = makeTask(
        "round trip", "2031-01-01 10:00", "low", "life", "2031-01-01 09:00");
    roundTrip.id = 42;
    const auto parsedRoundTrip = deserializeTask(serializeTask(roundTrip), parseError);
    passed = expect(parsedRoundTrip.has_value() && parsedRoundTrip->id == 42 &&
                        parsedRoundTrip->name == "round trip",
                    "a valid task should survive serialization") &&
             expect(!deserializeTask(
                        "12abc|bad|2031-01-01 10:00|medium|life|2031-01-01 09:00|0",
                        parseError)
                        .has_value(),
                    "stored ids with trailing characters should be rejected") &&
             passed;

    fs::remove_all(dataDirectory);
    unsetenv("MYSCHEDULE_DATA_DIR");
    return passed ? 0 : 1;
}
