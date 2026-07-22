#include "task.h"

#include "time_utils.h"

#include <sstream>
#include <vector>

namespace {
bool containsReservedDelimiter(const std::string& value) {
    return value.find('|') != std::string::npos || value.find('\n') != std::string::npos ||
           value.find('\r') != std::string::npos;
}

bool parseStoredTaskId(const std::string& value, int& id) {
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
        id = parsed;
        return true;
    } catch (...) {
        return false;
    }
}
}

bool isValidPriority(const std::string& priority) {
    return priority == "high" || priority == "medium" || priority == "low";
}

bool isValidCategory(const std::string& category) {
    return category == "study" || category == "entertainment" || category == "life";
}

bool validateTaskFields(const Task& task, std::string& errorMessage) {
    if (task.id < 0) {
        errorMessage = "Error: task id must be a non-negative integer.";
        return false;
    }
    if (task.name.empty()) {
        errorMessage = "Error: task name cannot be empty.";
        return false;
    }
    if (containsReservedDelimiter(task.name)) {
        errorMessage = "Error: task name cannot contain '|', carriage returns, or new lines.";
        return false;
    }
    if (!parseDateTime(task.startTime).has_value()) {
        errorMessage = "Error: invalid time format. Expected YYYY-MM-DD HH:MM.";
        return false;
    }
    if (!isValidPriority(task.priority)) {
        errorMessage = "Error: invalid priority. Expected high, medium, or low.";
        return false;
    }
    if (!isValidCategory(task.category)) {
        errorMessage = "Error: invalid category. Expected study, entertainment, or life.";
        return false;
    }
    if (!parseDateTime(task.remindTime).has_value()) {
        errorMessage = "Error: invalid remind time format. Expected YYYY-MM-DD HH:MM.";
        return false;
    }
    if (containsReservedDelimiter(task.priority) || containsReservedDelimiter(task.category) ||
        containsReservedDelimiter(task.startTime) || containsReservedDelimiter(task.remindTime)) {
        errorMessage = "Error: task fields cannot contain '|', carriage returns, or new lines.";
        return false;
    }
    if (compareDateTimes(task.remindTime, task.startTime) > 0) {
        errorMessage = "Error: remind time cannot be later than start time.";
        return false;
    }
    return true;
}

std::string serializeTask(const Task& task) {
    std::ostringstream output;
    output << task.id << '|'
           << task.name << '|'
           << task.startTime << '|'
           << task.priority << '|'
           << task.category << '|'
           << task.remindTime << '|'
           << (task.reminded ? 1 : 0);
    return output.str();
}

std::optional<Task> deserializeTask(const std::string& line, std::string& errorMessage) {
    std::vector<std::string> parts;
    std::stringstream stream(line);
    std::string part;
    while (std::getline(stream, part, '|')) {
        parts.push_back(part);
    }

    if (parts.size() != 7U) {
        errorMessage = "Error: invalid task record format.";
        return std::nullopt;
    }

    Task task;
    if (!parseStoredTaskId(parts[0], task.id)) {
        errorMessage = "Error: invalid task id in storage.";
        return std::nullopt;
    }

    task.name = parts[1];
    task.startTime = parts[2];
    task.priority = parts[3];
    task.category = parts[4];
    task.remindTime = parts[5];
    if (parts[6] != "0" && parts[6] != "1") {
        errorMessage = "Error: invalid reminded value in storage. Expected 0 or 1.";
        return std::nullopt;
    }
    task.reminded = (parts[6] == "1");

    if (!validateTaskFields(task, errorMessage)) {
        return std::nullopt;
    }
    return task;
}
