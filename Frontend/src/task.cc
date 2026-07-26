
#include "task.h"

/*
 * 文件职责：
 * 实现 Qt GUI 任务模型的时间转换、构造和调试输出。
 * 主界面创建任务时会根据日期时间控件和“提前提醒”控件构造本类对象。
 */

#include <iostream>
#include <utility>
using namespace std;

time_t task::to_time_t(int year, int month, int date, int hour, int minute,int second)
{
    // 将表单中的年月日时分秒转换为 time_t，供 Qt 表格展示和 CoreAdapter 转换。
    tm tm_time = {};
    tm_time.tm_year = year - 1900;
    tm_time.tm_mon = month - 1;
    tm_time.tm_mday = date;
    tm_time.tm_hour = hour;
    tm_time.tm_min = minute;
    tm_time.tm_sec = second;
    return mktime(&tm_time);
}
task::task(string name,
        int due_year, int due_month, int due_day, int due_hour, int due_minute,
        int remind_before_day, int remind_before_hour, int remind_before_minute, int remind_before_second,
        int id, string priority, string category, bool reminded)
        : task_name(std::move(name)), task_priority(std::move(priority)),
          task_category(std::move(category)), task_reminded(reminded), task_id(id) {
        task_due_time = to_time_t(due_year, due_month, due_day, due_hour, due_minute);
        // 提醒时间由“开始时间 - 提前提醒时长”计算，最终由核心层校验不能晚于开始时间。
        long long delta_remind_before_time = One_day_conversion * remind_before_day +
                                             One_hour_conversion * remind_before_hour +
                                             One_minute_conversion * remind_before_minute +
                                             remind_before_second;
        task_remind_time = task_due_time - static_cast<time_t>(delta_remind_before_time);

    }


void task::show() const
{
    if (task_id !=-1)
        cout << "任务[" << get_task_name() <<"]"<< endl;
    cout << "分类: " << get_task_category() << endl;
    cout << "优先级: " << get_task_priority() << endl;
    cout << "截止时间: " << ctime(&task_due_time);
    cout << "提醒时间: " << ctime(&task_remind_time);
}
