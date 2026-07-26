#ifndef MYSCHEDULE_VOICE_RESULT_H
#define MYSCHEDULE_VOICE_RESULT_H

/*
 * 文件职责：
 * 定义语音功能通用结果和错误码，便于 Qt 语音流程区分录音、STT、
 * DeepSeek 解析和任务创建等失败阶段。
 */

#include <string>

enum class VoiceErrorCode {
    None,
    FeatureDisabled,
    RecorderUnavailable,
    RecordingFailed,
    ServiceUnavailable,
    SttFailed,
    EmptyTranscript,
    ApiKeyMissing,
    DeepSeekRequestFailed,
    ParseFailed,
    InvalidTaskData,
    MissingRequiredField,
    UserCancelled,
    TaskCreationFailed
};

struct VoiceResult {
    bool success = false;
    VoiceErrorCode code = VoiceErrorCode::None;
    std::string message;
};

VoiceResult makeVoiceSuccess(std::string message = {});
VoiceResult makeVoiceError(VoiceErrorCode code, std::string message);
const char* voiceErrorCodeName(VoiceErrorCode code);

#endif
