#include "cli.h"

void CLI::printHelp(std::ostream& output) {
    output
        << "MySchedule - A command line schedule manager.\n\n"
        << "Usage:\n"
        << "  myschedule --help\n"
        << "  myschedule help\n\n"
        << "Commands:\n"
        << "  help        Show this help text.\n\n"
        << "Development status:\n"
        << "  Day 1 provides the project skeleton and help command.\n"
        << "  Accounts, task operations, storage, reminders, and Qt are not available yet.\n";
}
