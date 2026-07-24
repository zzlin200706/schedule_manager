#include "login_window.h"

#include "register_window.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPixmap>
#include <QPushButton>
#include <QSizePolicy>
#include <QVBoxLayout>

extern std::vector<task> tasks;

Login_Window::Login_Window(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("我的日程 — 日程管理器"));
    setMinimumSize(520, 630);
    resize(560, 680);

    auto* page = new QWidget(this);
    page->setObjectName("authPage");
    setCentralWidget(page);

    auto* pageLayout = new QVBoxLayout(page);
    pageLayout->setContentsMargins(32, 28, 32, 28);
    pageLayout->setSpacing(0);

    auto* card = new QFrame(page);
    card->setObjectName("authCard");
    card->setFixedWidth(450);
    card->setMinimumHeight(500);

    auto* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(40, 0, 40, 32);
    cardLayout->setSpacing(0);

    auto* accentRail = new QFrame(card);
    accentRail->setObjectName("accentRail");
    accentRail->setFixedHeight(4);
    cardLayout->addWidget(accentRail);
    cardLayout->addSpacing(24);

    auto* calendarIcon = new QLabel(card);
    calendarIcon->setObjectName("authCalendarIcon");
    calendarIcon->setPixmap(QPixmap(QStringLiteral(":/icons/calendar.svg")));
    calendarIcon->setAlignment(Qt::AlignHCenter);
    cardLayout->addWidget(calendarIcon);
    cardLayout->addSpacing(12);

    auto* titleLabel = new QLabel(QStringLiteral("我的日程"), card);
    titleLabel->setObjectName("authTitleLabel");
    titleLabel->setAlignment(Qt::AlignHCenter);
    cardLayout->addWidget(titleLabel);

    auto* subtitleLabel = new QLabel(QStringLiteral("管理今天，也规划明天"), card);
    subtitleLabel->setObjectName("secondaryLabel");
    subtitleLabel->setAlignment(Qt::AlignHCenter);
    cardLayout->addWidget(subtitleLabel);
    cardLayout->addSpacing(28);

    auto* usernameLabel = new QLabel(QStringLiteral("用户名"), card);
    usernameLabel->setObjectName("fieldLabel");
    usernameEdit = new QLineEdit(card);
    usernameEdit->setPlaceholderText(QStringLiteral("输入用户名"));
    usernameEdit->setClearButtonEnabled(false);
    cardLayout->addWidget(usernameLabel);
    cardLayout->addSpacing(8);
    cardLayout->addWidget(usernameEdit);
    cardLayout->addSpacing(18);

    auto* passwordLabel = new QLabel(QStringLiteral("密码"), card);
    passwordLabel->setObjectName("fieldLabel");
    passwordEdit = new QLineEdit(card);
    passwordEdit->setPlaceholderText(QStringLiteral("输入密码"));
    passwordEdit->setEchoMode(QLineEdit::Password);
    cardLayout->addWidget(passwordLabel);
    cardLayout->addSpacing(8);
    cardLayout->addWidget(passwordEdit);

    errorLabel = new QLabel(card);
    errorLabel->setObjectName("loginErrorLabel");
    errorLabel->setWordWrap(true);
    errorLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
    errorLabel->hide();
    cardLayout->addSpacing(10);
    cardLayout->addWidget(errorLabel);
    cardLayout->addSpacing(18);

    loginButton = new QPushButton(QStringLiteral("登录"), card);
    loginButton->setObjectName("primaryButton");
    loginButton->setDefault(false);
    loginButton->setAutoDefault(false);
    cardLayout->addWidget(loginButton);
    cardLayout->addSpacing(18);

    auto* registerRow = new QHBoxLayout;
    registerRow->setContentsMargins(0, 0, 0, 0);
    registerRow->setSpacing(8);
    auto* registerHint = new QLabel(QStringLiteral("还没有账号？"), card);
    registerHint->setObjectName("secondaryLabel");
    registerButton = new QPushButton(QStringLiteral("立即注册"), card);
    registerButton->setObjectName("linkButton");
    registerRow->addStretch();
    registerRow->addWidget(registerHint);
    registerRow->addWidget(registerButton);
    registerRow->addStretch();
    cardLayout->addLayout(registerRow);

    pageLayout->addStretch();
    pageLayout->addWidget(card, 0, Qt::AlignHCenter);
    pageLayout->addStretch();

    connect(loginButton, &QPushButton::clicked, this, &Login_Window::onLoginClicked);
    connect(registerButton, &QPushButton::clicked, this, &Login_Window::onRegisterClicked);
    connect(usernameEdit, &QLineEdit::returnPressed, this, &Login_Window::onLoginClicked);
    connect(passwordEdit, &QLineEdit::returnPressed, this, &Login_Window::onLoginClicked);
    connect(usernameEdit, &QLineEdit::textChanged, this, &Login_Window::clearError);
    connect(passwordEdit, &QLineEdit::textChanged, this, &Login_Window::clearError);

    usernameEdit->setFocus();
}

Login_Window::~Login_Window() = default;

void Login_Window::resetForm()
{
    loginInProgress_ = false;
    usernameEdit->clear();
    passwordEdit->clear();
    clearError();
    usernameEdit->setFocus();
}

void Login_Window::onLoginClicked()
{
    if (loginInProgress_) {
        return;
    }

    clearError();
    const QString username = usernameEdit->text().trimmed();
    const QString password = passwordEdit->text();

    if (username.isEmpty()) {
        showError(QStringLiteral("请输入用户名。"));
        usernameEdit->setFocus();
        return;
    }
    if (password.isEmpty()) {
        showError(QStringLiteral("请输入密码。"));
        passwordEdit->setFocus();
        return;
    }

    loginInProgress_ = true;
    sendLoginData(username, password);
}

void Login_Window::sendLoginData(const QString& username, const QString& password)
{
    const auto result = coreAdapter_.login(username, password);
    if (result.status == CoreAdapter::LoginStatus::Ok) {
        QString loadMessage;
        if (!coreAdapter_.loadTasks(username, tasks, loadMessage)) {
            QMessageBox::warning(this, QStringLiteral("任务加载失败"), loadMessage);
            tasks.clear();
        }
        clearError();
        emit loginSuccessful(username);
        return;
    }

    if (result.status == CoreAdapter::LoginStatus::UserNotFound) {
        loginInProgress_ = false;
        passwordEdit->clear();
        passwordEdit->setFocus();
        showError(QStringLiteral("用户不存在，请先注册后再登录。"));
        return;
    }

    if (result.status == CoreAdapter::LoginStatus::WrongPassword) {
        loginInProgress_ = false;
        passwordEdit->clear();
        passwordEdit->setFocus();
        showError(QStringLiteral("密码错误，请重新输入。"));
        return;
    }

    loginInProgress_ = false;
    showError(result.message.isEmpty() ? QStringLiteral("登录失败，请稍后重试。") : result.message);
}

void Login_Window::onRegisterClicked()
{
    Register_Window registerWindow(this);
    if (registerWindow.exec() != QDialog::Accepted) {
        return;
    }

    const QString username = registerWindow.registeredUsername();
    QString loadMessage;
    if (!coreAdapter_.loadTasks(username, tasks, loadMessage)) {
        QMessageBox::warning(this, QStringLiteral("任务加载失败"), loadMessage);
        tasks.clear();
    }
    emit loginSuccessful(username);
}

void Login_Window::showError(const QString& message)
{
    errorLabel->setText(message);
    errorLabel->show();
}

void Login_Window::clearError()
{
    errorLabel->clear();
    errorLabel->hide();
}
