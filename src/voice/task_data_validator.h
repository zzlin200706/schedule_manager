#ifndef MYSCHEDULE_TASK_DATA_VALIDATOR_H
#define MYSCHEDULE_TASK_DATA_VALIDATOR_H

#include "parsed_task_data.h"
#include "../core/task.h"
#include "voice_result.h"

#include <string>

struct ValidationResult {
    bool success = false;
    VoiceErrorCode code = VoiceErrorCode::InvalidTaskData;
    std::string message;
    Task task;
};

class TaskDataValidator {
public:
    ValidationResult validate(const ParsedTaskData& data) const;
};

#endif
