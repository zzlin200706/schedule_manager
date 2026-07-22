#include "time_utils.h"

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
    return dateTime.size() >= 10U && dateTime.substr(0, 10) == day;
}

bool sameMonth(const std::string& dateTime, const std::string& month) {
    return dateTime.size() >= 7U && dateTime.substr(0, 7) == month;
}
