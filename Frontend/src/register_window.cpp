#include "register_window.h"

/*
 * 文件职责：
 * 实现 Qt 注册对话框。用户输入用户名、密码和确认密码后，
 * 通过 CoreAdapter 调用核心层 UserManager::registerUser。
 *
 * 主要功能定位：
 * - 功能：Qt 注册界面
 * - 功能：用户注册
 */

#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPixmap>
#include <QPushButton>
#include <QVBoxLayout>

namespace {
void addCardShadow(QWidget* widget)
{
    auto* shadow = new QGraphicsDropShadowEffect(widget);
    shadow->setBlurRadius(18);
    shadow->setOffset(0, 4);
    shadow->setColor(QColor(28, 28, 30, 18));
    widget->setGraphicsEffect(shadow);
}
}

Register_Window::Register_Window(QWidget* parent)
    : QDialog(parent)
{
    // ==================== 功能：Qt 注册界面 ====================
    // 构造函数创建注册表单，并把“注册/返回登录”按钮连接到对应槽。
    setWindowTitle(QStringLiteral("注册账号 — 日程管理器"));
    setMinimumSize(520, 680);
    resize(560, 720);
    setModal(true);

    setObjectName("authDialog");
    auto* pageLayout = new QVBoxLayout(this);
    pageLayout->setContentsMargins(32, 28, 32, 28);
    pageLayout->setSpacing(0);

    auto* card = new QFrame(this);
    card->setObjectName("authCard");
    card->setFixedWidth(450);
    card->setMinimumHeight(570);
    addCardShadow(card);

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

    auto* titleLabel = new QLabel(QStringLiteral("创建日程账号"), card);
    titleLabel->setObjectName("authTitleLabel");
    titleLabel->setAlignment(Qt::AlignHCenter);
    cardLayout->addWidget(titleLabel);

    auto* subtitleLabel = new QLabel(QStringLiteral("使用同一账号保存你的任务与提醒"), card);
    subtitleLabel->setObjectName("secondaryLabel");
    subtitleLabel->setWordWrap(true);
    subtitleLabel->setAlignment(Qt::AlignHCenter);
    cardLayout->addWidget(subtitleLabel);
    cardLayout->addSpacing(26);

    auto addField = [card, cardLayout](const QString& labelText,
                                       const QString& placeholder,
                                       QLineEdit::EchoMode echoMode) {
        auto* label = new QLabel(labelText, card);
        label->setObjectName("fieldLabel");
        auto* edit = new QLineEdit(card);
        edit->setPlaceholderText(placeholder);
        edit->setEchoMode(echoMode);
        cardLayout->addWidget(label);
        cardLayout->addSpacing(8);
        cardLayout->addWidget(edit);
        cardLayout->addSpacing(16);
        return edit;
    };

    usernameEdit = addField(QStringLiteral("用户名"),
                            QStringLiteral("输入用户名"),
                            QLineEdit::Normal);
    usernameEdit->setClearButtonEnabled(true);
    passwordEdit1 = addField(QStringLiteral("密码"),
                             QStringLiteral("输入密码"),
                             QLineEdit::Password);
    passwordEdit2 = addField(QStringLiteral("确认密码"),
                             QStringLiteral("再次输入密码"),
                             QLineEdit::Password);

    errorLabel = new QLabel(card);
    errorLabel->setObjectName("formErrorLabel");
    errorLabel->setWordWrap(true);
    errorLabel->hide();
    cardLayout->addWidget(errorLabel);
    cardLayout->addSpacing(16);

    registerButton = new QPushButton(QStringLiteral("注册"), card);
    registerButton->setObjectName("primaryButton");
    registerButton->setDefault(true);
    registerButton->setAutoDefault(true);
    cardLayout->addWidget(registerButton);
    cardLayout->addSpacing(10);

    cancelButton = new QPushButton(QStringLiteral("返回登录"), card);
    cancelButton->setObjectName("secondaryButton");
    cardLayout->addWidget(cancelButton);

    pageLayout->addStretch();
    pageLayout->addWidget(card, 0, Qt::AlignHCenter);
    pageLayout->addStretch();

    connect(registerButton, &QPushButton::clicked, this, &Register_Window::onRegisterClicked);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    connect(usernameEdit, &QLineEdit::textChanged, this, &Register_Window::clearError);
    connect(passwordEdit1, &QLineEdit::textChanged, this, &Register_Window::clearError);
    connect(passwordEdit2, &QLineEdit::textChanged, this, &Register_Window::clearError);

    usernameEdit->setFocus();
}

Register_Window::~Register_Window() = default;

QString Register_Window::registeredUsername() const
{
    return registeredUsername_;
}

void Register_Window::onRegisterClicked()
{
    // 步骤 1：从输入框读取用户名和两次密码，先在界面层检查空值和一致性。
    const QString username = usernameEdit->text().trimmed();
    const QString password1 = passwordEdit1->text();
    const QString password2 = passwordEdit2->text();

    if (username.isEmpty()) {
        showError(QStringLiteral("请输入用户名。"));
        usernameEdit->setFocus();
        return;
    }
    if (password1.isEmpty()) {
        showError(QStringLiteral("请输入密码。"));
        passwordEdit1->setFocus();
        return;
    }
    if (password2.isEmpty()) {
        showError(QStringLiteral("请再次输入密码。"));
        passwordEdit2->setFocus();
        return;
    }
    if (password1 != password2) {
        showError(QStringLiteral("两次输入的密码不一致，请重新确认。"));
        passwordEdit2->selectAll();
        passwordEdit2->setFocus();
        return;
    }

    QString message;
    // 步骤 2：调用适配层注册；盐值生成、密码哈希和 users.txt 保存都在核心层完成。
    if (!coreAdapter_.registerUser(username, password1, message)) {
        showError(message.isEmpty() ? QStringLiteral("注册失败，请检查输入。") : message);
        return;
    }

    registeredUsername_ = username;
    accept();
}

void Register_Window::showError(const QString& message)
{
    errorLabel->setText(message);
    errorLabel->show();
}

void Register_Window::clearError()
{
    errorLabel->clear();
    errorLabel->hide();
}
