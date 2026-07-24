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
#ifdef ENABLE_VOICE_ASSISTANT
class QJsonObject;
class QNetworkAccessManager;
class QNetworkReply;
class QProcess;
#endif
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
#ifdef ENABLE_VOICE_ASSISTANT
    void on_voice_record_button_clicked();
    void onVoiceServiceStarted();
    void onVoiceServiceFinished(int exitCode, int exitStatus);
    void onVoiceServiceError();
    void onVoiceRecorderStarted();
    void onVoiceRecorderFinished(int exitCode, int exitStatus);
    void onVoiceRecorderError();
    void stopVoiceRecording(bool reachedTimeLimit);
#endif

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
#ifdef ENABLE_VOICE_ASSISTANT
    QPushButton* voice_record_button;
#endif
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
#ifdef ENABLE_VOICE_ASSISTANT
    QNetworkAccessManager* voiceNetworkManager;
    QProcess* voiceServiceProcess;
    QTimer* voiceRecordingTimer;
    QProcess* voiceRecordingProcess;
    QString voiceRecordingPath_;
    bool voiceServiceReady_ = false;
    bool voiceServiceOwnedProcess_ = false;
    bool voiceServiceStarting_ = false;
    bool voicePendingStartRecording_ = false;
    bool voiceTranscriptionActive_ = false;
    bool voiceParsingActive_ = false;
    int voiceServiceHealthAttempts_ = 0;
    bool voiceRecordingActive_ = false;
    bool voiceRecordingStopping_ = false;
#endif
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
#ifdef ENABLE_VOICE_ASSISTANT
    void ensureVoiceServiceThenStartRecording();
    void beginVoiceRecording();
    void requestVoiceServiceHealthCheck(bool allowStart);
    void handleVoiceServiceHealthReply(QNetworkReply* reply, bool allowStart);
    void startVoiceServiceProcess();
    void scheduleVoiceServiceHealthRetry();
    void failVoiceServiceStartup(const QString& message);
    QString voiceServiceDirectory() const;
    QString voiceServicePythonExecutable(const QString& serviceDirectory) const;
    void requestVoiceTranscription(const QString& audioPath);
    void handleVoiceTranscriptionReply(QNetworkReply* reply);
    void requestVoiceTaskParsing(const QString& transcript);
    void handleVoiceTaskParseReply(QNetworkReply* reply, const QString& transcript);
    bool fillTaskFormFromParsedVoiceTask(const QJsonObject& object,
                                         const QString& transcript,
                                         QString& message);
    bool applyPartialVoiceTaskFields(const QJsonObject& object,
                                     const QString& transcript);
    QString makeVoiceRecordingPath() const;
    void resetVoiceRecordingUi();
    void cleanupVoiceRecordingFile();
#endif
};

#endif // MAINWINDOW_H
