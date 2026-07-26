#include "task_data_validator.h"

/*
 * 文件职责：
 * 实现语音解析结果校验。把 DeepSeek 返回的 ParsedTaskData 转成核心层 Task，
 * 补齐默认优先级、分类和提醒时间，并复用核心任务字段校验。
 *
 * 主要功能定位：
 * - 功能：解析语音任务信息
 */

#include "time_utils.h"

namespace {
std::string trimmedOptionalValue(const std::optional<std::string>& value) {
    return value.has_value() ? trim(*value) : "";
}
}

ValidationResult TaskDataValidator::validate(const ParsedTaskData& data) const {
    // 步骤 1：裁剪自然语言解析出的字段，映射到核心层 Task。
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
        // 步骤 2：语音未识别优先级/分类/提醒时间时使用核心层默认值。
        task.priority = "medium";
    }
    if (task.category.empty()) {
        task.category = "life";
    }
    if (task.remindTime.empty()) {
        task.remindTime = task.startTime;
    }

    std::string errorMessage;
    // 步骤 3：复用核心层校验，确保语音任务和手动任务遵守同一规则。
    if (!validateTaskFields(task, errorMessage)) {
        return {false, VoiceErrorCode::InvalidTaskData, errorMessage, {}};
    }

    return {true, VoiceErrorCode::None, "Voice task data is valid.", task};
}
