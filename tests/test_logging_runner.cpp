/***************************************************************************
 *            test_logging_runner.cpp
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

/** @file test_logging_runner.cpp
 *  @brief Verify independent test-reporter and Logger command-line options.
 */

#include <sstream>
#include <string>
#include <vector>

#include "logging/test_runner.hpp"

#include "ariadne-testing.hpp"

using namespace Ariadne;

class TestLoggingRunner : public Ariadne::Testing::ContextFixture {
  public:
    using Ariadne::Testing::ContextFixture::ContextFixture;

    void test_options() {
        Logger& logger = Logger::instance();
        logger.use_blocking_scheduler();
        auto& configuration = logger.configuration();
        const unsigned int previous_verbosity = configuration.verbosity();
        const bool previous_multiline = configuration.handles_multiline_output();
        configuration.set_verbosity(10u);
        configuration.set_handles_multiline_output(true);

        std::ostringstream logged_output;
        std::streambuf* previous_buffer = std::clog.rdbuf(logged_output.rdbuf());

        unsigned int evaluations = 0u;
        Ariadne::Testing::Suite nested("runner options");
        nested.add("log expression", [&](Ariadne::Testing::Context& context) {
            LOGGING_PRINTLN("logging-runner-check-" << ++evaluations);
            context.prove(true, "suite executed");
        });

        auto run = [&](std::initializer_list<const char*> arguments, std::ostringstream& report) {
            std::vector<const char*> args(arguments);
            report.str("");
            report.clear();
            return Ariadne::Testing::run_logging_suite(
                nested, static_cast<int>(args.size()), args.data(), report);
        };

        std::ostringstream report;
        ARIADNE_TESTING_EXPECT_EQUAL(run({"test_logging_runner"}, report), 0);
        ARIADNE_TESTING_EXPECT_EQUAL(evaluations, 1u);
        ARIADNE_TESTING_PROVE(logged_output.str().find("logging-runner-check-1") != std::string::npos);
        ARIADNE_TESTING_PROVE(report.str().find("[RUN ") != std::string::npos);

        const std::size_t console_size = logged_output.str().size();
        ARIADNE_TESTING_EXPECT_EQUAL(
            run({"test_logging_runner", "--concise", "--logtonull"}, report), 0);
        ARIADNE_TESTING_EXPECT_EQUAL(evaluations, 2u);
        ARIADNE_TESTING_EXPECT_EQUAL(logged_output.str().size(), console_size);
        ARIADNE_TESTING_PROVE(report.str().find("[RUN ") == std::string::npos);
        ARIADNE_TESTING_PROVE(report.str().find("runner options") != std::string::npos);
        ARIADNE_TESTING_EXPECT_EQUAL(configuration.verbosity(), 10u);
        ARIADNE_TESTING_PROVE(configuration.handles_multiline_output());
        ARIADNE_TESTING_PROVE(std::clog.rdbuf() == logged_output.rdbuf());

        ARIADNE_TESTING_EXPECT_EQUAL(
            run({"test_logging_runner", "--full", "--logtonull"}, report), 0);
        ARIADNE_TESTING_EXPECT_EQUAL(evaluations, 3u);
        ARIADNE_TESTING_EXPECT_EQUAL(logged_output.str().size(), console_size);
        ARIADNE_TESTING_PROVE(report.str().find("[RUN ") != std::string::npos);

        ARIADNE_TESTING_EXPECT_EQUAL(run({"test_logging_runner", "--concise"}, report), 0);
        ARIADNE_TESTING_EXPECT_EQUAL(evaluations, 4u);
        ARIADNE_TESTING_PROVE(logged_output.str().find("logging-runner-check-4") != std::string::npos);
        ARIADNE_TESTING_PROVE(report.str().find("[RUN ") == std::string::npos);

        ARIADNE_TESTING_EXPECT_EQUAL(
            run({"test_logging_runner", "--logtonull", "--unknown"}, report), 2);
        ARIADNE_TESTING_EXPECT_EQUAL(evaluations, 4u);
        ARIADNE_TESTING_PROVE(report.str().find("Unknown test option") != std::string::npos);

        ARIADNE_TESTING_EXPECT_EQUAL(
            run({"test_logging_runner", "--help", "--logtonull"}, report), 0);
        ARIADNE_TESTING_EXPECT_EQUAL(evaluations, 4u);
        ARIADNE_TESTING_PROVE(report.str().find("--logtonull") != std::string::npos);

        std::clog.rdbuf(previous_buffer);
        configuration.set_verbosity(previous_verbosity);
        configuration.set_handles_multiline_output(previous_multiline);
    }
};

int main(int argc, char** argv) {
    Ariadne::Testing::Suite suite("logging runner");
    Ariadne::Testing::add_fixture_test<TestLoggingRunner>(
        suite, "test_options", &TestLoggingRunner::test_options);
    return Ariadne::Testing::run_suite(suite, argc, argv);
}
