#include "task_data_validator.h"

#include "time_utils.h"

namespace {
std::string trimmedOptionalValue(const std::optional<std::string>& value) {
    return value.has_value() ? trim(*value) : "";
}
}

ValidationResult TaskDataValidator::validate(const ParsedTaskData& data) const {
    Task task;
    task.name = trim(data.title);
    task.startTime = trimmedOptionalValue(data.startTime);
    task.priority = trimmedOptionalValue(data.priority);
    task.category = trimmedOptionalValue(data.category);
    task.remindTime = trimmedOptionalValue(data.reminderTime);

    if (task.name.empty()) {
        return {false,
                VoiceErrorCode::MissingRequiredField,
                "Voice task data is missing a task title.",
                {}};
    }
    if (task.startTime.empty()) {
        return {false,
                VoiceErrorCode::MissingRequiredField,
                "Voice task data is missing a start time.",
                {}};
    }
    if (task.priority.empty()) {
        task.priority = "medium";
    }
    if (task.category.empty()) {
        task.category = "life";
    }
    if (task.remindTime.empty()) {
        task.remindTime = task.startTime;
    }

    std::string errorMessage;
    if (!validateTaskFields(task, errorMessage)) {
        return {false, VoiceErrorCode::InvalidTaskData, errorMessage, {}};
    }

    return {true, VoiceErrorCode::None, "Voice task data is valid.", task};
}
