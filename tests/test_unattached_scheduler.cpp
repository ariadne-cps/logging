/***************************************************************************
 *            test_unattached_scheduler.cpp
 *
 *  Copyright  2026  Luca Geretti
 *
 ****************************************************************************/

/*
 *  This file is part of Ariadne Logging.
 *
 *  Ariadne Logging is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Ariadne Logging is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Ariadne Logging. If not, see <https://www.gnu.org/licenses/>.
 */

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
