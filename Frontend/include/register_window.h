#ifndef REGISTER_WINDOW_H
#define REGISTER_WINDOW_H


#include <QDialog>
#include <QString>

#include "core_adapter.h"

//注册窗口

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
