/***************************************************************************
 *            logging/test_runner.hpp
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

/** @file logging/test_runner.hpp
 *  @brief Test-only runner accepting --logtonull independently of --concise.
 */

#ifndef ARIADNE_LOGGING_TEST_RUNNER_HPP
#define ARIADNE_LOGGING_TEST_RUNNER_HPP

#include <ostream>
#include <string_view>
#include <vector>

#include "logging/logging.hpp"

#include "ariadne-testing.hpp"

namespace Ariadne::Testing {

namespace Detail {

/** @brief Redirect enabled Logger output to the platform null device for one suite.
 *  @details Run messages synchronously so that no large asynchronous queue
 *  remains after the testing reporter has completed its final summary.
 *  Preserve Logger verbosity and restore its stream and layout afterwards.
 */
class ScopedNullTestLogging {
  public:
    ScopedNullTestLogging()
        : _logger(Logger::instance()),
          _handles_multiline_output(_logger.configuration().handles_multiline_output()) {
        _logger.use_blocking_scheduler();
#ifdef _WIN32
        _logger.redirect_to_file("NUL");
#else
        _logger.redirect_to_file("/dev/null");
#endif
        _logger.configuration().set_handles_multiline_output(false);
    }

    ~ScopedNullTestLogging() {
        _logger.configuration().set_handles_multiline_output(_handles_multiline_output);
        _logger.redirect_to_console();
    }

    ScopedNullTestLogging(const ScopedNullTestLogging&) = delete;
    ScopedNullTestLogging& operator=(const ScopedNullTestLogging&) = delete;

  private:
    Logger& _logger;
    bool _handles_multiline_output;
};

} // namespace Detail

/** @brief Execute a Testing suite with an optional null Logger sink.
 *  @details --logtonull is stripped before forwarding the remaining arguments
 *  to Ariadne Testing. --full, --concise, and option validation stay with the
 *  existing Testing runner. Invoking the executable without --logtonull does
 *  not reconfigure Logger. No mute or verbosity change is performed.
 *
 *  @return Zero when all tests pass, one for a failed suite, or two when
 *  Ariadne Testing rejects an argument.
 */
template<class Argv>
int run_logging_suite(const Suite& suite, int argc, Argv argv, std::ostream& output = std::cout) {
    std::vector<const char*> testing_arguments;
    testing_arguments.reserve(static_cast<std::size_t>(argc));
    bool log_to_null = false;
    bool help = false;
    for (int i = 0; i < argc; ++i) {
        const char* argument = argv[i];
        if (i != 0 and argument != nullptr and std::string_view(argument) == "--logtonull") {
            log_to_null = true;
            continue;
        }
        if (i != 0 and argument != nullptr and std::string_view(argument) == "--help")
            help = true;
        testing_arguments.push_back(argument);
    }

    // Help does not configure the logger or run test cases.
    if (help) {
        const int result = run_suite(suite, static_cast<int>(testing_arguments.size()),
                                     testing_arguments.data(), output);
        output << "Logging option: --logtonull (discard Logger output)\n";
        return result;
    }

    if (not log_to_null)
        return run_suite(suite, static_cast<int>(testing_arguments.size()),
                         testing_arguments.data(), output);

    Detail::ScopedNullTestLogging logging;
    return run_suite(suite, static_cast<int>(testing_arguments.size()),
                     testing_arguments.data(), output);
}

} // namespace Ariadne::Testing

#endif // ARIADNE_LOGGING_TEST_RUNNER_HPP
