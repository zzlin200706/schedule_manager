#ifndef MYSCHEDULE_PARSED_TASK_DATA_H
#define MYSCHEDULE_PARSED_TASK_DATA_H

#include <optional>
#include <string>
#include <vector>

struct ParsedTaskData {
    std::string title;
    std::optional<std::string> startTime;
    std::optional<std::string> reminderTime;
    std::optional<std::string> priority;
    std::optional<std::string> category;
    std::vector<std::string> missingFields;
    std::string originalText;
};

#endif
