#include <QApplication>
#include <QByteArray>
#include <QCoreApplication>
#include <QFile>
#include <QFont>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QStringList>
#include "login_window.h"
#include "register_window.h"
#include "mainwindow.h"


std::vector<task> tasks;
string current_user_name;

namespace {
QString selectInterfaceFontFamily()
{
    const QStringList preferredFamilies = {
        QStringLiteral("Noto Sans CJK SC"),
        QStringLiteral("Source Han Sans SC"),
        QStringLiteral("Microsoft YaHei UI"),
        QStringLiteral("Microsoft YaHei"),
        QStringLiteral("PingFang SC"),
        QStringLiteral("WenQuanYi Micro Hei")
    };

    const QFontDatabase database;
    const QStringList availableFamilies = database.families();
    for (const QString& preferred : preferredFamilies) {
        for (const QString& available : availableFamilies) {
            if (available.compare(preferred, Qt::CaseInsensitive) == 0) {
                return available;
            }
        }
    }
    return {};
}

void loadApplicationStyle(QApplication& app)
{
    QFile styleFile(QStringLiteral(":/styles/workbench.qss"));
    if (!styleFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return;
    }

    app.setStyleSheet(QString::fromUtf8(styleFile.readAll()));
}
}

int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

    if (qgetenv("QT_QPA_PLATFORM").isEmpty()) {
        qputenv("QT_QPA_PLATFORM", QByteArray("xcb"));
    }
    if (qgetenv("QT_QPA_PLATFORM") != "wayland") {
        qputenv("XDG_SESSION_TYPE", QByteArray("x11"));
    }

    QApplication app(argc, argv);

    app.setApplicationName("ScheduleManagerGUI");
    QGuiApplication::setApplicationDisplayName(QString::fromUtf8("日程管理器"));
    QGuiApplication::setDesktopFileName("ScheduleManagerGUI");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("Schedule Manager Team");

    const QString interfaceFontFamily = selectInterfaceFontFamily();
    if (!interfaceFontFamily.isEmpty()) {
        QFont interfaceFont(interfaceFontFamily);
        interfaceFont.setStyleStrategy(QFont::PreferAntialias);
        app.setFont(interfaceFont);
    }

    loadApplicationStyle(app);
    
    MainWindow *mainWindow = nullptr;
    
    
    Login_Window loginWindow(nullptr);
    loginWindow.show();
    
    QObject::connect(&loginWindow, &Login_Window::loginSuccessful, 
                     [&](const QString &username) {
                         if (mainWindow != nullptr) {
                             return;
                         }

                         current_user_name= username.toStdString();
                        
                         mainWindow = new MainWindow(tasks, nullptr);
                         QObject::connect(mainWindow, &MainWindow::logoutRequested,
                                          [&]() {
                                              tasks.clear();
                                              current_user_name.clear();
                                              loginWindow.resetForm();
                                              loginWindow.show();
                                              loginWindow.raise();

                                              MainWindow* closingWindow = mainWindow;
                                              mainWindow = nullptr;
                                              closingWindow->close();
                                              closingWindow->deleteLater();
                                          });
                         mainWindow->show();
                         
                         
                         loginWindow.hide();
                     });
                     
    return app.exec();
}
