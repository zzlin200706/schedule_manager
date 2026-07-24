#include "mainwindow.h"

#include <QAbstractButton>
#include <QAbstractItemView>
#include <QCalendarWidget>
#include <QCloseEvent>
#include <QComboBox>
#include <QCoreApplication>
#include <QDateTimeEdit>
#include <QDialog>
#include <QFrame>
#include <QFont>
#include <QGraphicsDropShadowEffect>
#include <QGridLayout>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QSizePolicy>
#include <QSpinBox>
#include <QSplitter>
#include <QStackedWidget>
#include <QStyle>
#include <QStringList>
#include <QTableWidget>
#include <QTimer>
#include <QTextCharFormat>
#include <QUrl>
#include <QVBoxLayout>
#include <QWidget>

#include <algorithm>
#include <ctime>

extern std::string current_user_name;

namespace {
constexpr int kTableRowHeight = 44;

const QColor kHighPriorityBackground("#FFF0EF");
const QColor kHighPriorityText("#C92A24");
const QColor kMediumPriorityBackground("#FFF5E6");
const QColor kMediumPriorityText("#B45E00");
const QColor kLowPriorityBackground("#EAF8EF");
const QColor kLowPriorityText("#218C45");

void addCardShadow(QWidget* widget)
{
    auto* shadow = new QGraphicsDropShadowEffect(widget);
    shadow->setBlurRadius(14);
    shadow->setOffset(0, 3);
    shadow->setColor(QColor(28, 28, 30, 12));
    widget->setGraphicsEffect(shadow);
}

QString categoryText(const std::string& category)
{
    if (category == "study") {
        return QStringLiteral("学习");
    }
    if (category == "entertainment") {
        return QStringLiteral("娱乐");
    }
    if (category == "life") {
        return QStringLiteral("生活");
    }
    return QString::fromStdString(category);
}

QString priorityText(const std::string& priority)
{
    if (priority == "high") {
        return QStringLiteral("高");
    }
    if (priority == "medium") {
        return QStringLiteral("中");
    }
    if (priority == "low") {
        return QStringLiteral("低");
    }
    return QString::fromStdString(priority);
}

QTableWidgetItem* makeReadOnlyItem(const QString& text)
{
    auto* item = new QTableWidgetItem(text);
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    return item;
}

void selectComboData(QComboBox* combo, const QString& data)
{
    const int index = combo->findData(data);
    if (index >= 0) {
        combo->setCurrentIndex(index);
    }
}

void showReminderDialog(QWidget* parent, const task& reminder)
{
    const QString taskName = QString::fromStdString(reminder.get_task_name());
    const QString startTime = QDateTime::fromSecsSinceEpoch(reminder.get_task_due_time())
                                  .toString(QStringLiteral("yyyy-MM-dd HH:mm"));

    auto* dialog = new QDialog(parent);
    dialog->setObjectName("reminderDialog");
    dialog->setWindowTitle(QStringLiteral("日程提醒"));
    dialog->setWindowModality(Qt::NonModal);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setMinimumWidth(480);
    dialog->setMaximumWidth(560);

    auto* dialogLayout = new QVBoxLayout(dialog);
    dialogLayout->setContentsMargins(0, 0, 0, 0);
    dialogLayout->setSpacing(0);

    auto* panel = new QFrame(dialog);
    panel->setObjectName("reminderPanel");
    addCardShadow(panel);
    auto* panelLayout = new QHBoxLayout(panel);
    panelLayout->setContentsMargins(0, 0, 0, 0);
    panelLayout->setSpacing(0);

    auto* timeRail = new QFrame(panel);
    timeRail->setObjectName("reminderTimeRail");
    timeRail->setFixedWidth(7);
    panelLayout->addWidget(timeRail);

    auto* content = new QWidget(panel);
    content->setObjectName("reminderContent");
    auto* contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(22, 18, 22, 18);
    contentLayout->setSpacing(12);

    auto* headerLayout = new QHBoxLayout;
    headerLayout->setContentsMargins(0, 0, 0, 0);
    headerLayout->setSpacing(10);
    auto* label = new QLabel(QStringLiteral("日程提醒"), content);
    label->setObjectName("reminderLabel");
    auto* soundBadge = new QLabel(QStringLiteral("提醒已触发"), content);
    soundBadge->setObjectName("reminderSoundBadge");
    headerLayout->addWidget(label);
    headerLayout->addStretch();
    headerLayout->addWidget(soundBadge);
    contentLayout->addLayout(headerLayout);

    auto* title = new QLabel(QStringLiteral("“%1”即将开始").arg(taskName), content);
    title->setObjectName("reminderTitle");
    title->setWordWrap(true);
    contentLayout->addWidget(title);

    auto* timeLabel = new QLabel(startTime, content);
    timeLabel->setObjectName("reminderStartTime");
    contentLayout->addWidget(timeLabel);

    auto* detailLayout = new QHBoxLayout;
    detailLayout->setContentsMargins(0, 0, 0, 0);
    detailLayout->setSpacing(8);
    auto* categoryBadge = new QLabel(categoryText(reminder.get_task_category()), content);
    categoryBadge->setObjectName("reminderMetaBadge");
    auto* priorityBadge = new QLabel(
        QStringLiteral("%1优先级").arg(priorityText(reminder.get_task_priority())),
        content);
    priorityBadge->setObjectName("reminderMetaBadge");
    detailLayout->addWidget(categoryBadge);
    detailLayout->addWidget(priorityBadge);
    detailLayout->addStretch();
    contentLayout->addLayout(detailLayout);

    auto* actionLayout = new QHBoxLayout;
    actionLayout->setContentsMargins(0, 4, 0, 0);
    actionLayout->setSpacing(8);
    auto* closeButton = new QPushButton(QStringLiteral("知道了"), content);
    closeButton->setObjectName("reminderPrimaryButton");
    actionLayout->addStretch();
    actionLayout->addWidget(closeButton);
    contentLayout->addLayout(actionLayout);

    panelLayout->addWidget(content);
    dialogLayout->addWidget(panel);

    QObject::connect(closeButton, &QPushButton::clicked, dialog, &QDialog::accept);
    dialog->show();
}

}

MainWindow::MainWindow(const std::vector<task>& tasks, QWidget* parent)
    : QMainWindow(parent), current_user_tasks(tasks)
{
    resize(1280, 800);
    setMinimumSize(1080, 720);
    setWindowTitle(QStringLiteral("我的日程 — 日程管理器"));

    auto* page = new QWidget(this);
    page->setObjectName("workbenchPage");
    setCentralWidget(page);

    auto* pageLayout = new QVBoxLayout(page);
    pageLayout->setContentsMargins(20, 16, 20, 12);
    pageLayout->setSpacing(12);

    auto* topBar = new QWidget(page);
    topBar->setObjectName("topBar");
    auto* topBarLayout = new QHBoxLayout(topBar);
    topBarLayout->setContentsMargins(2, 0, 2, 0);
    topBarLayout->setSpacing(12);

    auto* titleLayout = new QVBoxLayout;
    titleLayout->setContentsMargins(0, 0, 0, 0);
    titleLayout->setSpacing(3);
    auto* pageTitle = new QLabel(QStringLiteral("我的日程"), topBar);
    pageTitle->setObjectName("pageTitleLabel");
    auto* pageSubtitle = new QLabel(QStringLiteral("安排当下，留出专注时间"), topBar);
    pageSubtitle->setObjectName("secondaryLabel");
    titleLayout->addWidget(pageTitle);
    titleLayout->addWidget(pageSubtitle);
    topBarLayout->addLayout(titleLayout);
    topBarLayout->addStretch();

    auto* userChip = new QFrame(topBar);
    userChip->setObjectName("userChip");
    auto* userChipLayout = new QHBoxLayout(userChip);
    userChipLayout->setContentsMargins(12, 7, 12, 7);
    userChipLayout->setSpacing(7);
    auto* userCaption = new QLabel(QStringLiteral("当前用户"), userChip);
    userCaption->setObjectName("utilityLabel");
    auto* userLabel = new QLabel(QString::fromStdString(current_user_name), userChip);
    userLabel->setObjectName("userNameLabel");
    userChipLayout->addWidget(userCaption);
    userChipLayout->addWidget(userLabel);
    topBarLayout->addWidget(userChip);

    auto* logoutButton = new QPushButton(QStringLiteral("退出登录"), topBar);
    logoutButton->setObjectName("secondaryButton");
    topBarLayout->addWidget(logoutButton);
    pageLayout->addWidget(topBar);

    auto* workspace = new QSplitter(Qt::Horizontal, page);
    workspace->setObjectName("workspaceSplitter");
    workspace->setChildrenCollapsible(false);
    workspace->setHandleWidth(16);

    leftScroll = new QScrollArea(workspace);
    leftScroll->setObjectName("leftScroll");
    leftScroll->setWidgetResizable(true);
    leftScroll->setFrameShape(QFrame::NoFrame);
    leftScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    leftScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    leftScroll->setMinimumWidth(410);
    leftScroll->setMaximumWidth(460);

    auto* leftColumn = new QWidget;
    leftColumn->setObjectName("leftColumn");
    leftColumn->setMinimumSize(394, 646);
    auto* leftLayout = new QVBoxLayout(leftColumn);
    leftLayout->setContentsMargins(6, 4, 8, 4);
    leftLayout->setSpacing(12);

    auto* calendarCard = new QFrame(leftColumn);
    calendarCard->setObjectName("workbenchCard");
    calendarCard->setMinimumWidth(394);
    calendarCard->setMinimumHeight(284);
    calendarCard->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
    addCardShadow(calendarCard);
    auto* calendarLayout = new QVBoxLayout(calendarCard);
    calendarLayout->setContentsMargins(18, 12, 18, 12);
    calendarLayout->setSpacing(6);

    auto* calendarHeader = new QHBoxLayout;
    calendarHeader->setContentsMargins(0, 0, 0, 0);
    calendarHeader->setSpacing(12);
    auto* calendarTitle = new QLabel(QStringLiteral("按日期查看"), calendarCard);
    calendarTitle->setObjectName("sectionTitleLabel");
    show_all_button = new QPushButton(QStringLiteral("全部任务"), calendarCard);
    show_all_button->setObjectName("compactButton");
    show_all_button->setEnabled(false);
    calendarHeader->addWidget(calendarTitle);
    calendarHeader->addStretch();
    calendarHeader->addWidget(show_all_button);
    calendarLayout->addLayout(calendarHeader);

    calendar = new QCalendarWidget(calendarCard);
    calendar->setObjectName("scheduleCalendar");
    calendar->setVerticalHeaderFormat(QCalendarWidget::NoVerticalHeader);
    calendar->setHorizontalHeaderFormat(QCalendarWidget::ShortDayNames);
    calendar->setGridVisible(false);
    calendar->setNavigationBarVisible(true);
    calendar->setLocale(QLocale(QLocale::Chinese, QLocale::China));
    calendar->setSelectedDate(QDate::currentDate());
    calendar->setMinimumWidth(356);
    calendar->setMinimumHeight(220);
    calendar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::MinimumExpanding);
    QTextCharFormat weekendFormat;
    weekendFormat.setForeground(QColor(QStringLiteral("#6E6E73")));
    calendar->setWeekdayTextFormat(Qt::Saturday, weekendFormat);
    calendar->setWeekdayTextFormat(Qt::Sunday, weekendFormat);
    QTextCharFormat todayFormat;
    todayFormat.setForeground(QColor(QStringLiteral("#007AFF")));
    todayFormat.setFontWeight(QFont::DemiBold);
    calendar->setDateTextFormat(QDate::currentDate(), todayFormat);
    calendarLayout->addWidget(calendar, 1);
    leftLayout->addWidget(calendarCard);

    auto* inputCard = new QFrame(leftColumn);
    inputCard->setObjectName("workbenchCard");
    inputCard->setMinimumHeight(342);
    addCardShadow(inputCard);
    auto* inputLayout = new QVBoxLayout(inputCard);
    inputLayout->setContentsMargins(16, 14, 16, 14);
    inputLayout->setSpacing(0);

    auto* inputTitle = new QLabel(QStringLiteral("安排任务"), inputCard);
    inputTitle->setObjectName("sectionTitleLabel");
    auto* inputHint = new QLabel(QStringLiteral("选中列表任务后，可在此保存修改"), inputCard);
    inputHint->setObjectName("helperLabel");
    inputLayout->addWidget(inputTitle);
    inputLayout->addSpacing(2);
    inputLayout->addWidget(inputHint);
    inputLayout->addSpacing(10);

    auto* formLayout = new QGridLayout;
    formLayout->setContentsMargins(0, 0, 0, 0);
    formLayout->setHorizontalSpacing(8);
    formLayout->setVerticalSpacing(6);
    formLayout->setColumnMinimumWidth(0, 58);
    formLayout->setColumnStretch(1, 1);
    formLayout->setColumnMinimumWidth(2, 48);
    formLayout->setColumnStretch(3, 1);

    const auto makeFieldLabel = [inputCard](const QString& text) {
        auto* label = new QLabel(text, inputCard);
        label->setObjectName("fieldLabel");
        label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        return label;
    };

    auto* nameLabel = makeFieldLabel(QStringLiteral("任务名称"));
    Task_input = new QLineEdit(inputCard);
    Task_input->setPlaceholderText(QStringLiteral("例如：复习高等数学第三章"));
    Task_input->setClearButtonEnabled(true);
    formLayout->addWidget(nameLabel, 0, 0);
    formLayout->addWidget(Task_input, 0, 1, 1, 3);

    auto* timeLabel = makeFieldLabel(QStringLiteral("开始时间"));
    due_time = new QDateTimeEdit(inputCard);
    due_time->setDisplayFormat(QStringLiteral("yyyy-MM-dd  HH:mm"));
    due_time->setCalendarPopup(true);
    due_time->setDateTime(QDateTime::currentDateTime());
    formLayout->addWidget(timeLabel, 1, 0);
    formLayout->addWidget(due_time, 1, 1, 1, 3);

    auto* categoryLabel = makeFieldLabel(QStringLiteral("分类"));
    categoryCombo = new QComboBox(inputCard);
    categoryCombo->addItem(QStringLiteral("学习"), QStringLiteral("study"));
    categoryCombo->addItem(QStringLiteral("娱乐"), QStringLiteral("entertainment"));
    categoryCombo->addItem(QStringLiteral("生活"), QStringLiteral("life"));
    selectComboData(categoryCombo, QStringLiteral("life"));
    formLayout->addWidget(categoryLabel, 2, 0);
    formLayout->addWidget(categoryCombo, 2, 1);

    auto* priorityLabel = makeFieldLabel(QStringLiteral("优先级"));
    priorityCombo = new QComboBox(inputCard);
    priorityCombo->addItem(QStringLiteral("高"), QStringLiteral("high"));
    priorityCombo->addItem(QStringLiteral("中"), QStringLiteral("medium"));
    priorityCombo->addItem(QStringLiteral("低"), QStringLiteral("low"));
    selectComboData(priorityCombo, QStringLiteral("medium"));
    formLayout->addWidget(priorityLabel, 2, 2);
    formLayout->addWidget(priorityCombo, 2, 3);

    auto* reminderLabel = makeFieldLabel(QStringLiteral("提前提醒"));
    auto* reminderLayout = new QHBoxLayout;
    reminderLayout->setContentsMargins(0, 0, 0, 0);
    reminderLayout->setSpacing(5);
    remind_before_day = new QSpinBox(inputCard);
    remind_before_day->setSuffix(QString());
    remind_before_day->setButtonSymbols(QAbstractSpinBox::NoButtons);
    remind_before_day->setAlignment(Qt::AlignCenter);
    remind_before_day->setFixedWidth(46);
    remind_before_hour = new QSpinBox(inputCard);
    remind_before_hour->setSuffix(QString());
    remind_before_hour->setButtonSymbols(QAbstractSpinBox::NoButtons);
    remind_before_hour->setAlignment(Qt::AlignCenter);
    remind_before_hour->setFixedWidth(46);
    remind_before_minute = new QSpinBox(inputCard);
    remind_before_minute->setSuffix(QString());
    remind_before_minute->setButtonSymbols(QAbstractSpinBox::NoButtons);
    remind_before_minute->setAlignment(Qt::AlignCenter);
    remind_before_minute->setFixedWidth(46);

    const auto addReminderUnit = [inputCard, reminderLayout](QSpinBox* spinBox,
                                                              const QString& unit) {
        auto* unitLabel = new QLabel(unit, inputCard);
        unitLabel->setObjectName("reminderUnitLabel");
        reminderLayout->addWidget(spinBox);
        reminderLayout->addWidget(unitLabel);
    };
    addReminderUnit(remind_before_day, QStringLiteral("天"));
    reminderLayout->addSpacing(8);
    addReminderUnit(remind_before_hour, QStringLiteral("小时"));
    reminderLayout->addSpacing(8);
    addReminderUnit(remind_before_minute, QStringLiteral("分钟"));
    reminderLayout->addStretch();
    formLayout->addWidget(reminderLabel, 3, 0);
    formLayout->addLayout(reminderLayout, 3, 1, 1, 3);
    inputLayout->addLayout(formLayout);
    inputLayout->addSpacing(8);

    add_task_button = new QPushButton(QStringLiteral("添加任务"), inputCard);
    add_task_button->setObjectName("primaryButton");
    inputLayout->addWidget(add_task_button);
    inputLayout->addSpacing(6);


    auto* editActions = new QHBoxLayout;
    editActions->setContentsMargins(0, 0, 0, 0);
    editActions->setSpacing(8);
    edit_task_button = new QPushButton(QStringLiteral("保存修改"), inputCard);
    edit_task_button->setObjectName("secondaryButton");
    cancel_task_button = new QPushButton(QStringLiteral("取消选择"), inputCard);
    cancel_task_button->setObjectName("quietButton");
    editActions->addWidget(edit_task_button, 1);
    editActions->addWidget(cancel_task_button, 1);
    inputLayout->addLayout(editActions);
    leftLayout->addWidget(inputCard, 1);

    auto* tableCard = new QFrame(workspace);
    tableCard->setObjectName("workbenchCard");
    tableCard->setMinimumWidth(600);
    addCardShadow(tableCard);
    auto* tableLayout = new QVBoxLayout(tableCard);
    tableLayout->setContentsMargins(20, 18, 20, 18);
    tableLayout->setSpacing(0);

    auto* tableHeader = new QHBoxLayout;
    tableHeader->setContentsMargins(0, 0, 0, 0);
    tableHeader->setSpacing(10);
    auto* tableTitle = new QLabel(QStringLiteral("任务清单"), tableCard);
    tableTitle->setObjectName("sectionTitleLabel");
    taskCountLabel = new QLabel(tableCard);
    taskCountLabel->setObjectName("countLabel");
    filterLabel = new QLabel(QStringLiteral("全部任务"), tableCard);
    filterLabel->setObjectName("filterPill");
    delete_task_button = new QPushButton(QStringLiteral("删除任务"), tableCard);
    delete_task_button->setObjectName("dangerButton");
    tableHeader->addWidget(tableTitle);
    tableHeader->addWidget(taskCountLabel);
    tableHeader->addStretch();
    tableHeader->addWidget(filterLabel);
    tableHeader->addWidget(delete_task_button);
    tableLayout->addLayout(tableHeader);

    feedbackLabel = new QLabel(tableCard);
    feedbackLabel->setObjectName("feedbackLabel");
    feedbackLabel->setWordWrap(true);
    feedbackLabel->hide();
    tableLayout->addSpacing(10);
    tableLayout->addWidget(feedbackLabel);
    tableLayout->addSpacing(10);

    taskStack = new QStackedWidget(tableCard);
    taskStack->setObjectName("taskStack");
    task_table = new QTableWidget(taskStack);
    task_table->setObjectName("taskTable");
    task_table->setColumnCount(5);
    task_table->setHorizontalHeaderLabels(
        {QStringLiteral("任务名称"), QStringLiteral("分类"), QStringLiteral("优先级"),
         QStringLiteral("开始时间"), QStringLiteral("提醒时间")});
    task_table->setAlternatingRowColors(true);
    task_table->setShowGrid(false);
    task_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    task_table->setSelectionMode(QAbstractItemView::SingleSelection);
    task_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    task_table->setWordWrap(false);
    task_table->setTextElideMode(Qt::ElideRight);
    task_table->setFocusPolicy(Qt::StrongFocus);
    task_table->verticalHeader()->setVisible(false);
    task_table->verticalHeader()->setDefaultSectionSize(kTableRowHeight);
    task_table->horizontalHeader()->setMinimumSectionSize(62);
    task_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    task_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    task_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
    task_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);
    task_table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Fixed);
    task_table->setColumnWidth(2, 72);
    task_table->setColumnWidth(3, 148);
    task_table->setColumnWidth(4, 148);

    auto* emptyState = new QWidget(taskStack);
    emptyState->setObjectName("emptyState");
    auto* emptyLayout = new QVBoxLayout(emptyState);
    emptyLayout->setContentsMargins(32, 32, 32, 32);
    emptyLayout->setSpacing(8);
    emptyTitleLabel = new QLabel(QStringLiteral("还没有任务"), emptyState);
    emptyTitleLabel->setObjectName("emptyTitleLabel");
    emptyTitleLabel->setAlignment(Qt::AlignCenter);
    emptyHintLabel = new QLabel(
        QStringLiteral("从左侧安排第一项任务，它会出现在这里。"), emptyState);
    emptyHintLabel->setObjectName("secondaryLabel");
    emptyHintLabel->setAlignment(Qt::AlignCenter);
    emptyHintLabel->setWordWrap(true);
    auto* emptyActionButton = new QPushButton(QStringLiteral("添加第一个任务"), emptyState);
    emptyActionButton->setObjectName("secondaryButton");
    emptyActionButton->setMaximumWidth(190);
    emptyLayout->addStretch();
    emptyLayout->addWidget(emptyTitleLabel);
    emptyLayout->addWidget(emptyHintLabel);
    emptyLayout->addSpacing(8);
    emptyLayout->addWidget(emptyActionButton, 0, Qt::AlignHCenter);
    emptyLayout->addStretch();

    taskStack->addWidget(task_table);
    taskStack->addWidget(emptyState);
    tableLayout->addWidget(taskStack, 1);

    leftScroll->setWidget(leftColumn);
    workspace->addWidget(leftScroll);
    workspace->addWidget(tableCard);
    workspace->setStretchFactor(0, 0);
    workspace->setStretchFactor(1, 1);
    workspace->setSizes({440, 760});
    pageLayout->addWidget(workspace, 1);

    connect(logoutButton, &QPushButton::clicked, this, &MainWindow::confirmLogout);
    connect(show_all_button, &QPushButton::clicked, this, &MainWindow::showAllTasks);
    connect(calendar, &QCalendarWidget::selectionChanged,
            this, &MainWindow::on_calendar_date_changed);
    connect(task_table, &QTableWidget::cellClicked,
            this, &MainWindow::on_task_table_cellClicked);
    connect(task_table, &QTableWidget::currentCellChanged,
            this, &MainWindow::on_task_table_cellClicked);
    connect(task_table, &QTableWidget::itemSelectionChanged,
            this, &MainWindow::updateSelectionActions);
    connect(add_task_button, &QPushButton::clicked,
            this, &MainWindow::on_add_task_button_clicked);
    connect(edit_task_button, &QPushButton::clicked,
            this, &MainWindow::on_edit_button_clicked);
    connect(cancel_task_button, &QPushButton::clicked,
            this, &MainWindow::clearTaskSelection);
    connect(delete_task_button, &QPushButton::clicked,
            this, &MainWindow::on_delete_task_button_clicked);
    connect(emptyActionButton, &QPushButton::clicked, Task_input,
            qOverload<>(&QWidget::setFocus));

    reminderTimer = new QTimer(this);
    connect(reminderTimer, &QTimer::timeout, this, &MainWindow::checkDueTasks);
    reminderTimer->start(10000);


    updateSelectionActions();
    refresh_task_table();
}

MainWindow::~MainWindow() = default;

void MainWindow::checkDueTasks()
{
    std::vector<task> dueReminders;
    QString message;
    if (!coreAdapter_.collectDueReminders(
            QString::fromStdString(current_user_name), dueReminders, message)) {
        showFeedback(QStringLiteral("提醒检查失败：%1").arg(message), "error");
        return;
    }

    for (const auto& reminder : dueReminders) {
        showReminderDialog(this, reminder);
    }
}

void MainWindow::on_calendar_date_changed()
{
    filteringByDate_ = true;
    selectedFilterDate_ = calendar->selectedDate();

    std::vector<task> tasksOfDay;
    for (const auto& currentTask : current_user_tasks) {
        const QDate taskDate = QDateTime::fromSecsSinceEpoch(currentTask.get_task_due_time()).date();
        if (taskDate == selectedFilterDate_) {
            tasksOfDay.push_back(currentTask);
        }
    }
    refresh_task_table(tasksOfDay);
}

void MainWindow::showAllTasks()
{
    filteringByDate_ = false;
    refresh_task_table();
}

void MainWindow::on_add_task_button_clicked()
{
    const QString name = Task_input->text().trimmed();
    if (name.isEmpty()) {
        showFeedback(QStringLiteral("请先输入任务名称。"), "error");
        Task_input->setFocus();
        return;
    }

    const QDateTime due = due_time->dateTime();
    const QDate date = due.date();
    const QTime time = due.time();
    const std::string priority = priorityCombo->currentData().toString().toStdString();
    const std::string category = categoryCombo->currentData().toString().toStdString();

    task newTask(name.toStdString(),
                 date.year(), date.month(), date.day(), time.hour(), time.minute(),
                 remind_before_day->value(), remind_before_hour->value(),
                 remind_before_minute->value(), 0, -1, priority, category);

    QString message;
    const QString username = QString::fromStdString(current_user_name);
    if (!coreAdapter_.addTask(username, newTask, message)) {
        showFeedback(QStringLiteral("添加失败：%1").arg(message), "error");
        return;
    }
    if (!reloadTasksFromCore(message)) {
        QMessageBox::warning(this, QStringLiteral("刷新失败"), message);
        return;
    }

    resetTaskForm();
    refresh_task_table();
    showFeedback(QStringLiteral("任务“%1”已添加。").arg(name), "success");
    Task_input->setFocus();
}

void MainWindow::refresh_task_table()
{
    if (filteringByDate_) {
        std::vector<task> tasksOfDay;
        for (const auto& currentTask : current_user_tasks) {
            if (QDateTime::fromSecsSinceEpoch(currentTask.get_task_due_time()).date()
                == selectedFilterDate_) {
                tasksOfDay.push_back(currentTask);
            }
        }
        refresh_task_table(tasksOfDay);
        return;
    }

    filterLabel->setText(QStringLiteral("全部任务"));
    show_all_button->setEnabled(false);
    emptyTitleLabel->setText(QStringLiteral("还没有任务"));
    emptyHintLabel->setText(
        QStringLiteral("从左侧安排第一项任务，它会出现在这里。"));
    populateTaskTable(current_user_tasks);
}

void MainWindow::refresh_task_table(const std::vector<task>& tasks)
{
    filterLabel->setText(selectedFilterDate_.toString(QStringLiteral("M月d日")));
    show_all_button->setEnabled(true);
    emptyTitleLabel->setText(QStringLiteral("这一天还没有任务"));
    emptyHintLabel->setText(
        QStringLiteral("可以从左侧为该日期安排任务，或切换回全部任务。"));
    populateTaskTable(tasks);
}

void MainWindow::populateTaskTable(const std::vector<task>& tasks)
{
    task_table->setUpdatesEnabled(false);
    task_table->clearContents();
    task_table->setRowCount(static_cast<int>(tasks.size()));

    for (int row = 0; row < static_cast<int>(tasks.size()); ++row) {
        const task& currentTask = tasks[static_cast<std::size_t>(row)];

        auto* nameItem = makeReadOnlyItem(QString::fromStdString(currentTask.get_task_name()));
        nameItem->setData(Qt::UserRole, currentTask.get_task_id());
        nameItem->setToolTip(nameItem->text());
        task_table->setItem(row, 0, nameItem);

        auto* categoryItem = makeReadOnlyItem(categoryText(currentTask.get_task_category()));
        categoryItem->setTextAlignment(Qt::AlignCenter);
        categoryItem->setToolTip(QString::fromStdString(currentTask.get_task_category()));
        task_table->setItem(row, 1, categoryItem);

        auto* priorityItem = makeReadOnlyItem(priorityText(currentTask.get_task_priority()));
        priorityItem->setTextAlignment(Qt::AlignCenter);
        priorityItem->setToolTip(QString::fromStdString(currentTask.get_task_priority()));
        if (currentTask.get_task_priority() == "high") {
            priorityItem->setBackground(kHighPriorityBackground);
            priorityItem->setForeground(kHighPriorityText);
        } else if (currentTask.get_task_priority() == "medium") {
            priorityItem->setBackground(kMediumPriorityBackground);
            priorityItem->setForeground(kMediumPriorityText);
        } else if (currentTask.get_task_priority() == "low") {
            priorityItem->setBackground(kLowPriorityBackground);
            priorityItem->setForeground(kLowPriorityText);
        }
        task_table->setItem(row, 2, priorityItem);

        const QDateTime due = QDateTime::fromSecsSinceEpoch(currentTask.get_task_due_time());
        auto* dueItem = makeReadOnlyItem(due.toString(QStringLiteral("yyyy-MM-dd HH:mm")));
        dueItem->setTextAlignment(Qt::AlignCenter);
        task_table->setItem(row, 3, dueItem);

        const QDateTime remind = QDateTime::fromSecsSinceEpoch(currentTask.get_task_remind_time());
        auto* remindItem = makeReadOnlyItem(remind.toString(QStringLiteral("yyyy-MM-dd HH:mm")));
        remindItem->setTextAlignment(Qt::AlignCenter);
        task_table->setItem(row, 4, remindItem);
    }

    task_table->setUpdatesEnabled(true);
    taskCountLabel->setText(QStringLiteral("共 %1 项").arg(tasks.size()));
    taskStack->setCurrentIndex(tasks.empty() ? 1 : 0);
    updateSelectionActions();
}

void MainWindow::on_delete_task_button_clicked()
{
    const int taskId = selectedTaskId();
    if (taskId <= 0) {
        showFeedback(QStringLiteral("请先选择要删除的任务。"), "error");
        return;
    }

    const int row = task_table->currentRow();
    const QString taskName = task_table->item(row, 0)->text();
    QMessageBox confirmation(QMessageBox::Warning,
                             QStringLiteral("删除任务"),
                             QStringLiteral("将永久删除“%1”，此操作无法撤销。").arg(taskName),
                             QMessageBox::NoButton,
                             this);
    QAbstractButton* deleteButton = confirmation.addButton(
        QStringLiteral("删除任务"), QMessageBox::DestructiveRole);
    deleteButton->setObjectName("dangerButton");
    confirmation.addButton(QStringLiteral("取消"), QMessageBox::RejectRole);
    confirmation.exec();
    if (confirmation.clickedButton() != deleteButton) {
        return;
    }

    QString message;
    const QString username = QString::fromStdString(current_user_name);
    if (!coreAdapter_.deleteTask(username, taskId, message)) {
        showFeedback(QStringLiteral("删除失败：%1").arg(message), "error");
        return;
    }
    if (!reloadTasksFromCore(message)) {
        QMessageBox::warning(this, QStringLiteral("刷新失败"), message);
        return;
    }

    resetTaskForm();
    refresh_task_table();
    showFeedback(QStringLiteral("任务“%1”已删除。").arg(taskName), "success");
}

void MainWindow::on_edit_button_clicked()
{
    const int taskId = selectedTaskId();
    if (taskId <= 0) {
        showFeedback(QStringLiteral("请先选择要修改的任务。"), "error");
        return;
    }

    const QString name = Task_input->text().trimmed();
    if (name.isEmpty()) {
        showFeedback(QStringLiteral("任务名称不能为空。"), "error");
        Task_input->setFocus();
        return;
    }

    const QDateTime due = due_time->dateTime();
    const QDate date = due.date();
    const QTime time = due.time();
    const std::string priority = priorityCombo->currentData().toString().toStdString();
    const std::string category = categoryCombo->currentData().toString().toStdString();
    task updatedTask(name.toStdString(),
                     date.year(), date.month(), date.day(), time.hour(), time.minute(),
                     remind_before_day->value(), remind_before_hour->value(),
                     remind_before_minute->value(), 0, taskId, priority, category);

    QString message;
    const QString username = QString::fromStdString(current_user_name);
    if (!coreAdapter_.updateTask(username, taskId, updatedTask, message)) {
        showFeedback(QStringLiteral("保存失败：%1").arg(message), "error");
        return;
    }
    if (!reloadTasksFromCore(message)) {
        QMessageBox::warning(this, QStringLiteral("刷新失败"), message);
        return;
    }

    resetTaskForm();
    refresh_task_table();
    showFeedback(QStringLiteral("任务“%1”已更新。").arg(name), "success");
}

void MainWindow::on_task_table_cellClicked(int row, int column)
{
    Q_UNUSED(column);
    if (row < 0) {
        return;
    }

    QTableWidgetItem* item = task_table->item(row, 0);
    if (item == nullptr) {
        return;
    }
    const int taskId = item->data(Qt::UserRole).toInt();
    const auto selected = std::find_if(
        current_user_tasks.begin(), current_user_tasks.end(),
        [taskId](const task& currentTask) { return currentTask.get_task_id() == taskId; });
    if (selected == current_user_tasks.end()) {
        return;
    }

    Task_input->setText(QString::fromStdString(selected->get_task_name()));
    const time_t dueTime = selected->get_task_due_time();
    due_time->setDateTime(QDateTime::fromSecsSinceEpoch(dueTime));
    selectComboData(categoryCombo, QString::fromStdString(selected->get_task_category()));
    selectComboData(priorityCombo, QString::fromStdString(selected->get_task_priority()));

    const time_t remindTime = selected->get_task_remind_time();
    int deltaSeconds = static_cast<int>(difftime(dueTime, remindTime));
    if (deltaSeconds < 0) {
        deltaSeconds = 0;
    }
    remind_before_day->setValue(deltaSeconds / 86400);
    remind_before_hour->setValue((deltaSeconds % 86400) / 3600);
    remind_before_minute->setValue((deltaSeconds % 3600) / 60);
    updateSelectionActions();
    leftScroll->ensureWidgetVisible(edit_task_button, 0, 18);
}

void MainWindow::resetTaskForm()
{
    task_table->clearSelection();
    task_table->setCurrentItem(nullptr);
    Task_input->clear();
    due_time->setDateTime(QDateTime::currentDateTime());
    remind_before_day->setValue(0);
    remind_before_hour->setValue(0);
    remind_before_minute->setValue(0);
    selectComboData(categoryCombo, QStringLiteral("life"));
    selectComboData(priorityCombo, QStringLiteral("medium"));
    updateSelectionActions();
    leftScroll->ensureWidgetVisible(Task_input, 0, 18);
}

void MainWindow::clearTaskSelection()
{
    resetTaskForm();
    showFeedback(QStringLiteral("已取消任务选择。"));
    Task_input->setFocus();
}

void MainWindow::updateSelectionActions()
{
    const bool hasSelection = selectedTaskId() > 0;
    edit_task_button->setEnabled(hasSelection);
    delete_task_button->setEnabled(hasSelection);
    cancel_task_button->setEnabled(hasSelection);
}

int MainWindow::selectedTaskId() const
{
    const int row = task_table->currentRow();
    if (row < 0) {
        return -1;
    }
    const QTableWidgetItem* item = task_table->item(row, 0);
    if (item == nullptr) {
        return -1;
    }
    bool ok = false;
    const int taskId = item->data(Qt::UserRole).toInt(&ok);
    return ok ? taskId : -1;
}

void MainWindow::showFeedback(const QString& message, const char* kind)
{
    feedbackLabel->setProperty("kind", kind);
    feedbackLabel->setText(message);
    feedbackLabel->style()->unpolish(feedbackLabel);
    feedbackLabel->style()->polish(feedbackLabel);
    feedbackLabel->show();
}

void MainWindow::confirmLogout()
{
    reminderTimer->stop();
    emit logoutRequested();
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    reminderTimer->stop();
    event->accept();
}

bool MainWindow::reloadTasksFromCore(QString& message)
{
    return coreAdapter_.loadTasks(
        QString::fromStdString(current_user_name), current_user_tasks, message);
}
