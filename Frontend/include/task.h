#ifndef TASK_H
#define TASK_H
#include<iostream>
#include<fstream>
#include<string>
#include<ctime>
#include<algorithm>
#include<functional>
#include<vector>
#include <QString>
#include <QMetaType>
#include <QJsonObject>
using namespace std;

//task类定义
// 任务类用于存储和管理任务信息
// 包含任务名称、分类、优先级、截止时间、提醒状态和任务ID等属性

class task{
private:
    string task_name;
    string task_priority;
    string task_category;
    time_t task_due_time;
    time_t task_remind_time;
    bool task_reminded;
    int task_id;

    enum {
        One_day_conversion = 24 * 60 * 60,
        One_hour_conversion = 60 * 60,
        One_minute_conversion = 60
    };

private:
time_t to_time_t(int year, int month, int date, int hour, int minute, int second = 0);

public:
    // 构造函数
    task()
        : task_priority("medium"), task_category("life"), task_due_time(0),
          task_remind_time(0), task_reminded(false), task_id(-1) {}
    task(string name,
        int due_year, int due_month, int due_day, int due_hour, int due_minute,
        int remind_before_day = 0, int remind_before_hour = 0, int remind_before_minute = 0, int remind_before_second = 0,
        int id = -1, string priority = "medium", string category = "life", bool reminded = false);

    // 获取任务信息
    string get_task_name() const { return task_name; }
    string get_task_priority() const { return task_priority; }
    string get_task_category() const { return task_category; }
    time_t get_task_due_time() const { return task_due_time; }
    time_t get_task_remind_time() const { return task_remind_time; }
    bool get_task_reminded() const { return task_reminded; }
    int get_task_id() const { return task_id; }

    bool operator==(const task& other) const {
        //task_id == other.task_id &&
        return task_name == other.task_name &&
               task_priority == other.task_priority &&
               task_category == other.task_category &&
               task_due_time == other.task_due_time &&
               task_remind_time == other.task_remind_time &&
               task_reminded == other.task_reminded;
    }




    void show() const;
    ~task(){}

    // 与task与json文件的互转
    friend inline task task_from_json(const QJsonObject& obj);
    friend inline QJsonObject task_to_json(const task& t);
};


inline task task_from_json(const QJsonObject& obj)
{
    task t;
    t.task_name = obj["task_name"].toString().toStdString();
    t.task_priority = obj["task_priority"].toString("medium").toStdString();
    t.task_category = obj["task_category"].toString("life").toStdString();
    t.task_due_time = static_cast<time_t>(obj["task_due_time"].toDouble());
    t.task_remind_time = static_cast<time_t>(obj["task_remind_time"].toDouble());
    t.task_reminded = obj["task_reminded"].toBool(false);
    t.task_id = obj["task_id"].toInt();
    return t;
}

inline QJsonObject task_to_json(const task& t)
{
    QJsonObject obj;
    obj["task_name"] = QString::fromStdString(t.get_task_name());
    obj["task_priority"] = QString::fromStdString(t.get_task_priority());
    obj["task_category"] = QString::fromStdString(t.get_task_category());
    obj["task_due_time"] = static_cast<double>(t.get_task_due_time());
    obj["task_remind_time"] = static_cast<double>(t.get_task_remind_time());
    obj["task_reminded"] = t.get_task_reminded();
    obj["task_id"] = t.get_task_id();
    return obj;
}
#endif
