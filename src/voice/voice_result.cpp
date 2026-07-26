#include "voice_result.h"

/*
 * 文件职责：
 * 实现语音功能通用结果构造和错误码名称转换。
 */

#include <utility>

VoiceResult makeVoiceSuccess(std::string message) {
    return {true, VoiceErrorCode::None, std::move(message)};
}

VoiceResult makeVoiceError(VoiceErrorCode code, std::string message) {
    return {false, code, std::move(message)};
}

const char* voiceErrorCodeName(VoiceErrorCode code) {
    switch (code) {
    case VoiceErrorCode::None:
        return "None";
    case VoiceErrorCode::FeatureDisabled:
        return "FeatureDisabled";
    case VoiceErrorCode::RecorderUnavailable:
        return "RecorderUnavailable";
    case VoiceErrorCode::RecordingFailed:
        return "RecordingFailed";
    case VoiceErrorCode::ServiceUnavailable:
        return "ServiceUnavailable";
    case VoiceErrorCode::SttFailed:
        return "SttFailed";
    case VoiceErrorCode::EmptyTranscript:
        return "EmptyTranscript";
    case VoiceErrorCode::ApiKeyMissing:
        return "ApiKeyMissing";
    case VoiceErrorCode::DeepSeekRequestFailed:
        return "DeepSeekRequestFailed";
    case VoiceErrorCode::ParseFailed:
        return "ParseFailed";
    case VoiceErrorCode::InvalidTaskData:
        return "InvalidTaskData";
    case VoiceErrorCode::MissingRequiredField:
        return "MissingRequiredField";
    case VoiceErrorCode::UserCancelled:
        return "UserCancelled";
    case VoiceErrorCode::TaskCreationFailed:
        return "TaskCreationFailed";
    }
    return "Unknown";
}
