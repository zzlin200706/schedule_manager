#ifndef MYSCHEDULE_PARSED_TASK_DATA_H
#define MYSCHEDULE_PARSED_TASK_DATA_H

/*
 * 文件职责：
 * 定义语音服务解析后的中间任务数据。Qt 从 /parse-task 得到 JSON 后，
 * 会转换为本结构，再交给 TaskDataValidator 生成核心层 Task。
 */

#include <optional>
#include <string>
#include <vector>

/**
 * @brief 语音解析后的任务字段
 *
 * optional 字段表示 DeepSeek 可能没有识别出对应信息；
 * missingFields 用于提示用户需要在界面中补充哪些字段。
 */
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
