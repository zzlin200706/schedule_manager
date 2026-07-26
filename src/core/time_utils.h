#ifndef MYSCHEDULE_TIME_UTILS_H
#define MYSCHEDULE_TIME_UTILS_H

/*
 * 文件职责：
 * 声明日期时间解析、日期/月筛选、字符串裁剪和 CLI 命令分词工具。
 * 这些工具服务于任务校验、排序、查询和交互式命令解析。
 */

#include <ctime>
#include <optional>
#include <string>
#include <vector>

std::optional<std::tm> parseDateTime(const std::string& value);
bool isValidDayString(const std::string& value);
bool isValidMonthString(const std::string& value);
std::time_t toTimeT(const std::tm& value);
int compareDateTimes(const std::string& left, const std::string& right);
bool sameDay(const std::string& dateTime, const std::string& day);
bool sameMonth(const std::string& dateTime, const std::string& month);
std::string trim(const std::string& value);
bool tokenizeCommandLine(const std::string& line,
                         std::vector<std::string>& tokens,
                         std::string& errorMessage);

#endif
