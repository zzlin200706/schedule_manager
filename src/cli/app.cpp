#include "app.h"

#include "cli.h"

#include <iostream>

int App::run(const std::vector<std::string>& args) {
    if (!args.empty() && (args[0] == "--help" || args[0] == "help")) {
        CLI::printHelp(std::cout);
        return 0;
    }

    if (args.empty()) {
        std::cerr << "Error: no command was provided.\n";
    } else {
        std::cerr << "Error: unknown command '" << args[0] << "'.\n";
    }
    std::cerr << "Run `myschedule --help` for usage.\n";
    return 1;
}
