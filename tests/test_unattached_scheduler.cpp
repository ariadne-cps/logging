// Standalone logging must not need ThreadManager or a synthetic registry.
#include "logging/logging.hpp"
#include "logging/logging_command_line_interface.hpp"

int main() {
    using namespace Ariadne;
    auto& logger = Logger::instance();
    if (logger.has_thread_registry_attached()) return 1;
    LoggingCommandLineInterface cli;
    if (not cli.acquire({"logging-test", "--verbosity", "1",
                         "--scheduler", "immediate"})) return 2;
    LOGGING_PRINTLN("immediate without registry")
    if (not cli.acquire({"logging-test", "--scheduler", "blocking"})) return 3;
    LOGGING_PRINTLN("blocking without registry")
    if (not cli.acquire({"logging-test", "--scheduler", "nonblocking"})) return 4;
    LOGGING_PRINTLN("nonblocking without registry")
    if (logger.has_thread_registry_attached()) return 5;
    return 0;
}
