#ifndef LOGIN_WINDOW_H
#define LOGIN_WINDOW_H

/*
 * 文件职责：
 * 声明 Qt 登录窗口。窗口收集用户名和密码，通过 CoreAdapter 调用核心层，
 * 登录成功后发出 loginSuccessful 信号交给 main.cpp 切换主界面。
 */

#include <QMainWindow>
#include <QString>
#include <vector>

#include "core_adapter.h"
#include "task.h"

// 前向声明
class Register_Window;

/**
 * @brief Qt 登录界面
 *
 * 只负责界面输入校验、错误提示和信号发送；用户验证和任务加载通过
 * CoreAdapter 转发到核心层。
 */
class Login_Window : public QMainWindow
{
    Q_OBJECT

public:
    explicit Login_Window(QWidget *parent=nullptr);
    ~Login_Window();
    void resetForm();

signals:
    void loginSuccessful(const QString &username);

private slots:
    void onLoginClicked();
    void onRegisterClicked();

private:
    class QLineEdit *usernameEdit;
    class QLineEdit *passwordEdit;
    class QLabel *errorLabel;
    class QPushButton *loginButton;
    class QPushButton *registerButton;

    CoreAdapter coreAdapter_;
    bool loginInProgress_ = false;

    void sendLoginData(const QString &username, const QString &password);
    void showError(const QString& message);
    void clearError();
};
#endif // LOGIN_WINDOW_H
