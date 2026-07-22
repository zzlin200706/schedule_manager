#ifndef MYSCHEDULE_TIME_UTILS_H
#define MYSCHEDULE_TIME_UTILS_H

#include <ctime>
#include <optional>
#include <string>

std::optional<std::tm> parseDateTime(const std::string& value);
bool isValidDayString(const std::string& value);
bool isValidMonthString(const std::string& value);
std::time_t toTimeT(const std::tm& value);
int compareDateTimes(const std::string& left, const std::string& right);
bool sameDay(const std::string& dateTime, const std::string& day);
bool sameMonth(const std::string& dateTime, const std::string& month);

#endif
