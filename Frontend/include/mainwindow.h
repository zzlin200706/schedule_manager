#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QDate>
#include <QMainWindow>

#include <vector>

#include "core_adapter.h"

class QCalendarWidget;
class QCloseEvent;
class QComboBox;
class QDateTimeEdit;
class QLabel;
class QLineEdit;
class QPushButton;
class QScrollArea;
class QSpinBox;
class QStackedWidget;
class QTableWidget;
class QTimer;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(const std::vector<task>& tasks, QWidget* parent = nullptr);
    ~MainWindow() override;
    void closeEvent(QCloseEvent* event) override;

signals:
    void logoutRequested();

private slots:
    void on_add_task_button_clicked();
    void on_delete_task_button_clicked();
    void on_edit_button_clicked();
    void on_task_table_cellClicked(int row, int column);
    void on_calendar_date_changed();
    void showAllTasks();
    void clearTaskSelection();
    void confirmLogout();
    void checkDueTasks();

private:
    QCalendarWidget* calendar;
    QDateTimeEdit* due_time;
    QSpinBox* remind_before_day;
    QSpinBox* remind_before_hour;
    QSpinBox* remind_before_minute;
    QLineEdit* Task_input;
    QComboBox* categoryCombo;
    QComboBox* priorityCombo;
    QPushButton* add_task_button;
    QPushButton* edit_task_button;
    QPushButton* delete_task_button;
    QPushButton* cancel_task_button;
    QPushButton* show_all_button;
    QLabel* taskCountLabel;
    QLabel* filterLabel;
    QLabel* feedbackLabel;
    QLabel* emptyTitleLabel;
    QLabel* emptyHintLabel;
    QScrollArea* leftScroll;
    QTableWidget* task_table;
    QStackedWidget* taskStack;

    std::vector<task> current_user_tasks;
    QTimer* reminderTimer;
    CoreAdapter coreAdapter_;
    bool filteringByDate_ = false;
    QDate selectedFilterDate_;

    void refresh_task_table(const std::vector<task>& task_list);
    void refresh_task_table();
    bool reloadTasksFromCore(QString& message);
    void populateTaskTable(const std::vector<task>& tasks);
    void resetTaskForm();
    void updateSelectionActions();
    void showFeedback(const QString& message, const char* kind = "info");
    int selectedTaskId() const;
};

#endif // MAINWINDOW_H
