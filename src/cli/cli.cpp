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
        << "  The project skeleton, help command, and core account storage are available.\n"
        << "  Interactive login, task operations, reminders, and Qt are not available yet.\n";
}
