#ifndef LOGIN_WINDOW_H
#define LOGIN_WINDOW_H


#include <QMainWindow>
#include <QString>
#include <vector>

#include "core_adapter.h"
#include "task.h"

// 前向声明
class Register_Window;

// 登陆窗口

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
