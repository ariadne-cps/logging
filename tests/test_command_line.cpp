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

#include "logging/command_line.hpp"
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

        ARIADNE_TEST_CALL(test_contribution())
        ARIADNE_TEST_CALL(test_scheduler())
        ARIADNE_TEST_CALL(test_theme())
        ARIADNE_TEST_CALL(test_verbosity())
        ARIADNE_TEST_CALL(test_combined())
        ARIADNE_TEST_CALL(test_scheduler_priority())
    }

  private:
    CommandLineInterface make_cli() const {
        CommandLineInterface cli;
        cli.add(logging_command_line());
        return cli;
    }

    Void test_contribution() {
        CommandLineContribution const contribution=logging_command_line();
        ARIADNE_TEST_EQUALS(contribution.name(),"logging")
        ARIADNE_TEST_EQUALS(contribution.parsers().size(),3u)
        ARIADNE_TEST_EQUALS(contribution.parsers()[0].long_id(),"scheduler")
        ARIADNE_TEST_EQUALS(contribution.parsers()[1].long_id(),"theme")
        ARIADNE_TEST_EQUALS(contribution.parsers()[2].long_id(),"verbosity")
    }

    Void test_scheduler() {
        _registry.set_threads_registered(false);
        CommandLineInterface cli=make_cli();

        ARIADNE_TEST_ASSERT(cli.acquire({"","-s","immediate"}))
        ARIADNE_TEST_ASSERT(cli.acquire({"","--scheduler","blocking"}))
        ARIADNE_TEST_ASSERT(cli.acquire({"","-s","nonblocking"}))
        ARIADNE_TEST_ASSERT(not cli.acquire({"","-s","wrong"}))
        ARIADNE_TEST_ASSERT(not cli.acquire({"","-s"}))
    }

    Void test_theme() {
        CommandLineInterface cli=make_cli();

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
        CommandLineInterface cli=make_cli();

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
        CommandLineInterface cli=make_cli();

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

        CommandLineInterface cli=make_cli();
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
