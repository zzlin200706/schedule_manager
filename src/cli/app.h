#ifndef MYSCHEDULE_APP_H
#define MYSCHEDULE_APP_H

/*
 * 文件职责：
 * 声明 CLI 应用控制器。App 负责把命令行参数或交互式输入
 * 分发到用户管理、任务管理和提醒线程。
 */

#include <string>
#include <vector>

/**
 * @brief CLI 应用入口控制类
 *
 * run() 是 main.cpp 进入 CLI 业务的唯一入口；交互模式和带账号密码的
 * 直接命令模式分别由私有函数处理。
 */
class App {
public:
    int run(const std::vector<std::string>& args);

private:
    int runInteractiveMode();
    int runAuthenticatedCommand(const std::vector<std::string>& args);
};

#endif
