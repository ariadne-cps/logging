/***************************************************************************
 *            test_command_line.cpp
 *
 *  Copyright  2026  Luca Geretti
 *
 ****************************************************************************/

/*
 *  This file is part of Ariadne Logging.
 *
 *  Logging is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 */

#include "logging/logging_command_line_interface.hpp"
#include "logging/logging.hpp"
#include "macro/testing.hpp"

using namespace Ariadne;

class CommandLineThreadRegistry : public ThreadRegistryInterface {
  public:
    Bool has_threads_registered() const override {
        return _has_threads_registered;
    }

    Void set_threads_registered(Bool value) {
        _has_threads_registered=value;
    }

  private:
    Bool _has_threads_registered=false;
};

class TestCommandLine {
  public:
    Void test() {
        Logger::instance().attach_thread_registry(&_registry);

        ARIADNE_TEST_CALL(test_inherited_composition())
        ARIADNE_TEST_CALL(test_scheduler())
        ARIADNE_TEST_CALL(test_theme())
        ARIADNE_TEST_CALL(test_verbosity())
        ARIADNE_TEST_CALL(test_combined())
        ARIADNE_TEST_CALL(test_scheduler_priority())
    }

  private:
    LoggingCommandLineInterface make_cli() const {
        return LoggingCommandLineInterface();
    }

    Void test_inherited_composition() {
        LoggingCommandLineInterface cli;
        ARIADNE_TEST_ASSERT(cli.acquire({""}))
        ARIADNE_TEST_ASSERT(cli.acquire({"","--verbosity","3"}))
        ARIADNE_TEST_EQUALS(Logger::instance().configuration().verbosity(),3u)

        Bool custom_flag=false;
        cli.add(CommandLineContribution("custom", {
            make_unvalued_argument_parser(
                "x","custom-flag","Exercise inherited contribution support",
                [&custom_flag] { custom_flag=true; })
        }));
        ARIADNE_TEST_ASSERT(cli.acquire({"","--custom-flag","--theme","none"}))
        ARIADNE_TEST_ASSERT(custom_flag)
    }

    Void test_scheduler() {
        _registry.set_threads_registered(false);
        LoggingCommandLineInterface cli=make_cli();

        ARIADNE_TEST_ASSERT(cli.acquire({"","-s","immediate"}))
        ARIADNE_TEST_ASSERT(cli.acquire({"","--scheduler","blocking"}))
        ARIADNE_TEST_ASSERT(cli.acquire({"","-s","nonblocking"}))
        ARIADNE_TEST_ASSERT(not cli.acquire({"","-s","wrong"}))
        ARIADNE_TEST_ASSERT(not cli.acquire({"","-s"}))
    }

    Void test_theme() {
        LoggingCommandLineInterface cli=make_cli();

        ARIADNE_TEST_ASSERT(cli.acquire({"","-t","none"}))
        ARIADNE_TEST_ASSERT(not Logger::instance().configuration().theme().has_style())

        ARIADNE_TEST_ASSERT(cli.acquire({"","--theme","light"}))
        ARIADNE_TEST_ASSERT(Logger::instance().configuration().theme().has_style())

        ARIADNE_TEST_ASSERT(cli.acquire({"","-t","dark"}))
        ARIADNE_TEST_ASSERT(Logger::instance().configuration().theme().has_style())

        ARIADNE_TEST_ASSERT(not cli.acquire({"","-t","wrong"}))
        ARIADNE_TEST_ASSERT(not cli.acquire({"","-t"}))
    }

    Void test_verbosity() {
        LoggingCommandLineInterface cli=make_cli();

        ARIADNE_TEST_ASSERT(cli.acquire({"","-v","5"}))
        ARIADNE_TEST_EQUALS(Logger::instance().configuration().verbosity(),5u)

        ARIADNE_TEST_ASSERT(cli.acquire({"","--verbosity","0"}))
        ARIADNE_TEST_EQUALS(Logger::instance().configuration().verbosity(),0u)

        ARIADNE_TEST_ASSERT(not cli.acquire({"","-v","-2"}))
        ARIADNE_TEST_ASSERT(not cli.acquire({"","-v","q"}))
        ARIADNE_TEST_ASSERT(not cli.acquire({"","-v"}))
    }

    Void test_combined() {
        _registry.set_threads_registered(false);
        LoggingCommandLineInterface cli=make_cli();

        ARIADNE_TEST_ASSERT(cli.acquire({
            "",
            "--verbosity","4",
            "--theme","none",
            "--scheduler","immediate"
        }))
        ARIADNE_TEST_EQUALS(Logger::instance().configuration().verbosity(),4u)
        ARIADNE_TEST_ASSERT(not Logger::instance().configuration().theme().has_style())
    }

    Void test_scheduler_priority() {
        _registry.set_threads_registered(false);

        LoggingCommandLineInterface cli=make_cli();
        cli.add(CommandLineContribution(
            "test",
            {
                make_unvalued_argument_parser(
                    "r",
                    "register-thread",
                    "Pretend a thread is registered",
                    [this] { _registry.set_threads_registered(true); })
            }));

        // Although the test action appears first on the command line, the
        // scheduler action must run first because changing scheduler is
        // forbidden once a thread is registered.
        ARIADNE_TEST_ASSERT(cli.acquire({
            "",
            "--register-thread",
            "--scheduler","blocking"
        }))
        ARIADNE_TEST_ASSERT(_registry.has_threads_registered())

        _registry.set_threads_registered(false);
        Logger::instance().use_nonblocking_scheduler();
    }

  private:
    CommandLineThreadRegistry _registry;
};

Int main() {
    TestCommandLine().test();
    return ARIADNE_TEST_FAILURES;
}
