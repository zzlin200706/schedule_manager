#ifndef REGISTER_WINDOW_H
#define REGISTER_WINDOW_H

/*
 * 文件职责：
 * 声明 Qt 注册对话框。对话框校验两次密码输入，并通过 CoreAdapter
 * 调用核心层创建用户。
 */

#include <QDialog>
#include <QString>

#include "core_adapter.h"

/**
 * @brief Qt 注册界面
 *
 * 注册成功后保存新用户名，并以 Accepted 结果返回登录窗口。
 */
class Register_Window : public QDialog
{
    Q_OBJECT

public:
    explicit Register_Window(QWidget *parent = nullptr);
    ~Register_Window();
    QString registeredUsername() const;

private slots:
    void onRegisterClicked();


private:
    class QLineEdit *usernameEdit;
    class QLineEdit *passwordEdit1;
    class QLineEdit *passwordEdit2;
    class QLabel *errorLabel;
    class QPushButton *registerButton;
    class QPushButton *cancelButton;

    CoreAdapter coreAdapter_;
    QString registeredUsername_;

    void showError(const QString& message);
    void clearError();
};
#endif // REGISTER_WINDOW_H
