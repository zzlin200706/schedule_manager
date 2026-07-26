#ifndef MYSCHEDULE_TASK_DATA_VALIDATOR_H
#define MYSCHEDULE_TASK_DATA_VALIDATOR_H

/*
 * 文件职责：
 * 声明语音解析结果到核心层 Task 的校验器。它把可选字段补默认值，
 * 再复用核心层 validateTaskFields 做最终校验。
 *
 * 主要功能定位：
 * - 功能：解析语音任务信息
 */

#include "parsed_task_data.h"
#include "../core/task.h"
#include "voice_result.h"

#include <string>

/**
 * @brief 语音任务数据校验结果
 *
 * success 为 true 时 task 字段可交给界面填表；失败时 code/message
 * 说明缺少字段或字段不满足核心任务规则。
 */
struct ValidationResult {
    bool success = false;
    VoiceErrorCode code = VoiceErrorCode::InvalidTaskData;
    std::string message;
    Task task;
};

/**
 * @brief 语音解析结果校验器
 */
class TaskDataValidator {
public:
    ValidationResult validate(const ParsedTaskData& data) const;
};

#endif
