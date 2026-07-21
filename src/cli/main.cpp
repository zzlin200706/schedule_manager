#include "app.h"

#include <string>
#include <vector>

int main(int argc, char* argv[]) {
    std::vector<std::string> args;
    for (int index = 1; index < argc; ++index) {
        args.emplace_back(argv[index]);
    }

    App app;
    return app.run(args);
}
