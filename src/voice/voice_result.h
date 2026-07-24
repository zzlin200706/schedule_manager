#ifndef MYSCHEDULE_VOICE_RESULT_H
#define MYSCHEDULE_VOICE_RESULT_H

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
