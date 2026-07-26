#include "time_utils.h"

/*
 * 文件职责：
 * 实现统一的时间格式校验、任务排序比较、按日期/月筛选辅助函数，
 * 以及交互式 CLI 的命令行分词。
 *
 * 主要功能定位：
 * - 功能：任务排序
 * - 功能：按日期查看任务
 * - 功能：按月份查看任务
 * - CLI 引号/转义解析由 tokenizeCommandLine 处理
 */

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <sstream>

namespace {
bool parseWithFormat(const std::string& value, const char* format, std::tm& parsed) {
    std::istringstream stream(value);
    stream >> std::get_time(&parsed, format);
    if (stream.fail()) {
        return false;
    }

    char extra = '\0';
    if (stream >> extra) {
        return false;
    }

    std::tm normalized = parsed;
    std::time_t stamp = std::mktime(&normalized);
    if (stamp == static_cast<std::time_t>(-1)) {
        return false;
    }

    std::ostringstream output;
    output << std::put_time(&normalized, format);
    // mktime 会自动归一化非法日期，这里反向格式化确认输入没有被“纠正”。
    if (output.str() != value) {
        return false;
    }

    parsed = normalized;
    return true;
}
}

std::optional<std::tm> parseDateTime(const std::string& value) {
    std::tm parsed = {};
    if (!parseWithFormat(value, "%Y-%m-%d %H:%M", parsed)) {
        return std::nullopt;
    }
    return parsed;
}

bool isValidDayString(const std::string& value) {
    std::tm parsed = {};
    return parseWithFormat(value, "%Y-%m-%d", parsed);
}

bool isValidMonthString(const std::string& value) {
    if (value.size() != 7U || value[4] != '-') {
        return false;
    }
    std::string candidate = value + "-01";
    return isValidDayString(candidate);
}

std::time_t toTimeT(const std::tm& value) {
    std::tm copy = value;
    return std::mktime(&copy);
}

int compareDateTimes(const std::string& left, const std::string& right) {
    const auto leftTm = parseDateTime(left);
    const auto rightTm = parseDateTime(right);
    if (!leftTm.has_value() || !rightTm.has_value()) {
        return left.compare(right);
    }

    const std::time_t leftStamp = toTimeT(*leftTm);
    const std::time_t rightStamp = toTimeT(*rightTm);
    if (leftStamp < rightStamp) {
        return -1;
    }
    if (leftStamp > rightStamp) {
        return 1;
    }
    return 0;
}

bool sameDay(const std::string& dateTime, const std::string& day) {
    // 核心层存储格式固定为 YYYY-MM-DD HH:MM，因此日期筛选可直接比较前 10 位。
    return dateTime.size() >= 10U && dateTime.substr(0, 10) == day;
}

bool sameMonth(const std::string& dateTime, const std::string& month) {
    // 月份筛选比较 YYYY-MM 前缀，调用方先保证 month 格式有效。
    return dateTime.size() >= 7U && dateTime.substr(0, 7) == month;
}

std::string trim(const std::string& value) {
    const auto first = std::find_if_not(value.begin(), value.end(), [](unsigned char ch) {
        return std::isspace(ch) != 0;
    });
    if (first == value.end()) {
        return "";
    }

    const auto last = std::find_if_not(value.rbegin(), value.rend(), [](unsigned char ch) {
        return std::isspace(ch) != 0;
    }).base();
    return std::string(first, last);
}

bool tokenizeCommandLine(const std::string& line,
                         std::vector<std::string>& tokens,
                         std::string& errorMessage) {
    // 支持单引号、双引号和反斜杠转义，让交互式 CLI 可输入带空格的任务名。
    tokens.clear();
    std::string token;
    char quote = '\0';
    bool escaping = false;
    bool tokenStarted = false;

    for (const unsigned char rawCh : line) {
        const char ch = static_cast<char>(rawCh);
        if (escaping) {
            token.push_back(ch);
            tokenStarted = true;
            escaping = false;
            continue;
        }

        if (ch == '\\' && quote != '\'') {
            escaping = true;
            tokenStarted = true;
            continue;
        }

        if (quote != '\0') {
            if (ch == quote) {
                quote = '\0';
            } else {
                token.push_back(ch);
            }
            tokenStarted = true;
            continue;
        }

        if (ch == '\'' || ch == '"') {
            quote = ch;
            tokenStarted = true;
            continue;
        }

        if (std::isspace(rawCh) != 0) {
            if (tokenStarted) {
                tokens.push_back(token);
                token.clear();
                tokenStarted = false;
            }
            continue;
        }

        token.push_back(ch);
        tokenStarted = true;
    }

    if (escaping) {
        errorMessage = "Error: command cannot end with an escape character.";
        tokens.clear();
        return false;
    }
    if (quote != '\0') {
        errorMessage = "Error: unmatched quote in command.";
        tokens.clear();
        return false;
    }
    if (tokenStarted) {
        tokens.push_back(token);
    }
    errorMessage.clear();
    return true;
}
