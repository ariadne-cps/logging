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
#include "ariadne-testing.hpp"

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

class TestCommandLine : public Ariadne::Testing::ContextFixture {
  public:
    explicit TestCommandLine(Ariadne::Testing::Context& ctx) : ContextFixture(ctx) { }
    Void test() {
        Logger::instance().attach_thread_registry(&_registry);

        do { auto phase_scope=this->ctx().scope("phase", "test_inherited_composition()"); ARIADNE_TESTING_EXPECT_NO_THROW( test_inherited_composition()); } while(false);
        do { auto phase_scope=this->ctx().scope("phase", "test_scheduler()"); ARIADNE_TESTING_EXPECT_NO_THROW( test_scheduler()); } while(false);
        do { auto phase_scope=this->ctx().scope("phase", "test_theme()"); ARIADNE_TESTING_EXPECT_NO_THROW( test_theme()); } while(false);
        do { auto phase_scope=this->ctx().scope("phase", "test_verbosity()"); ARIADNE_TESTING_EXPECT_NO_THROW( test_verbosity()); } while(false);
        do { auto phase_scope=this->ctx().scope("phase", "test_combined()"); ARIADNE_TESTING_EXPECT_NO_THROW( test_combined()); } while(false);
        do { auto phase_scope=this->ctx().scope("phase", "test_scheduler_priority()"); ARIADNE_TESTING_EXPECT_NO_THROW( test_scheduler_priority()); } while(false);
    }

  private:
    LoggingCommandLineInterface make_cli() const {
        return LoggingCommandLineInterface();
    }

    Void test_inherited_composition() {
        LoggingCommandLineInterface cli;
        ARIADNE_TESTING_PROVE( cli.acquire({""}));
        ARIADNE_TESTING_PROVE( cli.acquire({"","--verbosity","3"}));
        ARIADNE_TESTING_EXPECT_EQUAL( Logger::instance().configuration().verbosity(), 3u);

        Bool custom_flag=false;
        cli.add(CommandLineContribution("custom", {
            make_unvalued_argument_parser(
                "x","custom-flag","Exercise inherited contribution support",
                [&custom_flag] { custom_flag=true; })
        }));
        ARIADNE_TESTING_PROVE( cli.acquire({"","--custom-flag","--theme","none"}));
        ARIADNE_TESTING_PROVE( custom_flag);
    }

    Void test_scheduler() {
        _registry.set_threads_registered(false);
        LoggingCommandLineInterface cli=make_cli();

        ARIADNE_TESTING_PROVE( cli.acquire({"","-s","immediate"}));
        ARIADNE_TESTING_PROVE( cli.acquire({"","--scheduler","blocking"}));
        ARIADNE_TESTING_PROVE( cli.acquire({"","-s","nonblocking"}));
        ARIADNE_TESTING_PROVE( not cli.acquire({"","-s","wrong"}));
        ARIADNE_TESTING_PROVE( not cli.acquire({"","-s"}));
    }

    Void test_theme() {
        LoggingCommandLineInterface cli=make_cli();

        ARIADNE_TESTING_PROVE( cli.acquire({"","-t","none"}));
        ARIADNE_TESTING_PROVE( not Logger::instance().configuration().theme().has_style());

        ARIADNE_TESTING_PROVE( cli.acquire({"","--theme","light"}));
        ARIADNE_TESTING_PROVE( Logger::instance().configuration().theme().has_style());

        ARIADNE_TESTING_PROVE( cli.acquire({"","-t","dark"}));
        ARIADNE_TESTING_PROVE( Logger::instance().configuration().theme().has_style());

        ARIADNE_TESTING_PROVE( not cli.acquire({"","-t","wrong"}));
        ARIADNE_TESTING_PROVE( not cli.acquire({"","-t"}));
    }

    Void test_verbosity() {
        LoggingCommandLineInterface cli=make_cli();

        ARIADNE_TESTING_PROVE( cli.acquire({"","-v","5"}));
        ARIADNE_TESTING_EXPECT_EQUAL( Logger::instance().configuration().verbosity(), 5u);

        ARIADNE_TESTING_PROVE( cli.acquire({"","--verbosity","0"}));
        ARIADNE_TESTING_EXPECT_EQUAL( Logger::instance().configuration().verbosity(), 0u);

        ARIADNE_TESTING_PROVE( not cli.acquire({"","-v","-2"}));
        ARIADNE_TESTING_PROVE( not cli.acquire({"","-v","q"}));
        ARIADNE_TESTING_PROVE( not cli.acquire({"","-v"}));
    }

    Void test_combined() {
        _registry.set_threads_registered(false);
        LoggingCommandLineInterface cli=make_cli();

        ARIADNE_TESTING_PROVE( cli.acquire({
            "",
            "--verbosity","4",
            "--theme","none",
            "--scheduler","immediate"
        }));
        ARIADNE_TESTING_EXPECT_EQUAL( Logger::instance().configuration().verbosity(), 4u);
        ARIADNE_TESTING_PROVE( not Logger::instance().configuration().theme().has_style());
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
        ARIADNE_TESTING_PROVE( cli.acquire({
            "",
            "--register-thread",
            "--scheduler","blocking"
        }));
        ARIADNE_TESTING_PROVE( _registry.has_threads_registered());

        _registry.set_threads_registered(false);
        Logger::instance().use_nonblocking_scheduler();
    }

  private:
    CommandLineThreadRegistry _registry;
};

int main() {
    Ariadne::Testing::Suite suite("logging-command-line");
    suite.add("command line", [](Ariadne::Testing::Context& ctx) {
        TestCommandLine(ctx).test();
    });
    return Ariadne::Testing::run_suite(suite);
}
