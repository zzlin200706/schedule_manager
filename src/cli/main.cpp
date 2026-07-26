#include "app.h"

/*
 * 文件职责：
 * CLI 可执行程序入口。这里只负责收集命令行参数并交给 App，
 * 具体登录、注册、命令分发和任务管理逻辑在 app.cpp 中实现。
 *
 * 主要功能定位：
 * - 功能：程序入口
 */

#include <string>
#include <vector>

// ==================== 功能：程序入口 ====================
int main(int argc, char* argv[]) {
    std::vector<std::string> args;
    for (int index = 1; index < argc; ++index) {
        args.emplace_back(argv[index]);
    }

    App app;
    return app.run(args);
}
