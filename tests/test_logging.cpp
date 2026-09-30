/***************************************************************************
 *            test_logging.cpp
 *
 *  Copyright  2021  Luca Geretti
 *
 ****************************************************************************/

/*
 *  This file is part of Logging.
 *
 *  Logging is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Logging is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Logging.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <list>
#include <array>
#ifndef _WIN32
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#endif
#include "thread.hpp"
#include "logging/logging.hpp"
#include "logging/progress_indicator.hpp"
#include "utility/test.hpp"

using namespace Ariadne;

void sample_function() {
    LOGGING_SCOPE_CREATE
    LOGGING_PRINTLN("val=inf, x0=2.0^3*1.32424242432423[2,3], y>[0.1:0.2] (z={0:1}), 1, x0, x11, true@1.")
}

void sample_printhold_simple_loop(std::string txt, SizeType u) {
    LOGGING_SCOPE_CREATE
    for (unsigned int i=0; i<3; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(u));
        LOGGING_SCOPE_PRINTHOLD(txt<<"@"<<i)
    }
}

void sample_printhold_nested_loop(std::string txt, SizeType u) {
    LOGGING_SCOPE_CREATE
    for (unsigned int i=0; i<3; ++i) {
        sample_printhold_simple_loop("internal",u);
        std::this_thread::sleep_for(std::chrono::milliseconds(u));
        LOGGING_SCOPE_PRINTHOLD(txt<<"@"<<i)
    }
}

void print_something1() {
    LOGGING_PRINTLN("This is a call from thread id " << std::this_thread::get_id() << " named '" << Logger::instance().current_thread_name() << "'")
}

void print_something2() {
    LOGGING_SCOPE_CREATE
    LOGGING_PRINTLN("This is a call from thread id " << std::this_thread::get_id() << " named '" << Logger::instance().current_thread_name() << "'")
}

class ThreadRegistry : public ThreadRegistryInterface {
  public:
    ThreadRegistry() : _threads_registered(0) { }
    bool has_threads_registered() const override { return _threads_registered > 0; }
    void set_threads_registered(unsigned int threads_registered) { _threads_registered = threads_registered; }
  private:
    unsigned int _threads_registered;
};

class TestLogging {
  private:
    ThreadRegistry _registry;
  public:
    TestLogging() {
        Logger::instance().configuration().set_prints_level_on_change_only(false);
    }

    void test() {
        ARIADNE_TEST_CALL(test_thread_registry())
        ARIADNE_TEST_CALL(test_print_configuration())
        ARIADNE_TEST_CALL(test_style_branch_combinations())
        ARIADNE_TEST_CALL(test_parser_branch_boundaries())
        ARIADNE_TEST_CALL(test_stylecode_adjacency_helper())
        ARIADNE_TEST_CALL(test_scheduler_noop_registration_paths())
        ARIADNE_TEST_CALL(test_hold_release_missing_scope())
        ARIADNE_TEST_CALL(test_remaining_branch_boundaries())
        ARIADNE_TEST_CALL(test_window_columns())
        ARIADNE_TEST_CALL(test_shown_single_print())
        ARIADNE_TEST_CALL(test_hidden_single_print())
        ARIADNE_TEST_CALL(test_muted_print())
        ARIADNE_TEST_CALL(test_use_blocking_scheduler())
        ARIADNE_TEST_CALL(test_use_nonblocking_scheduler())
        ARIADNE_TEST_CALL(test_shown_call_function_with_entrance_and_exit())
        ARIADNE_TEST_CALL(test_hide_call_function_with_entrance_and_exit())
        ARIADNE_TEST_CALL(test_indents_based_on_level())
        ARIADNE_TEST_CALL(test_high_level_multiline_hold())
        ARIADNE_TEST_CALL(test_high_level_hidden_level_and_hold_reprint())
        ARIADNE_TEST_CALL(test_hold_line())
        ARIADNE_TEST_CALL(test_hold_line_with_newline_println())
        ARIADNE_TEST_CALL(test_hold_long_line())
        ARIADNE_TEST_CALL(test_hold_multiple())
        ARIADNE_TEST_CALL(test_light_theme())
        ARIADNE_TEST_CALL(test_dark_theme())
        ARIADNE_TEST_CALL(test_theme_custom_keyword())
        ARIADNE_TEST_CALL(test_theme_bgcolor_bold_underline())
        ARIADNE_TEST_CALL(test_handles_multiline_output())
        ARIADNE_TEST_CALL(test_discards_newlines_and_indentation())
        ARIADNE_TEST_CALL(test_redirect())
        ARIADNE_TEST_CALL(test_multiple_threads_with_blocking_scheduler())
        ARIADNE_TEST_CALL(test_multiple_threads_with_nonblocking_scheduler())
        ARIADNE_TEST_CALL(test_register_self_thread())
        ARIADNE_TEST_CALL(test_printing_policy_with_theme_and_print_level(true,true))
        ARIADNE_TEST_CALL(test_printing_policy_with_theme_and_print_level(true,false))
        ARIADNE_TEST_CALL(test_printing_policy_with_theme_and_print_level(false,false))
        ARIADNE_TEST_CALL(test_printing_policy_with_theme_and_print_level(false,true))
    }

    void test_thread_registry() {
        ARIADNE_TEST_FAIL(Logger::instance().use_immediate_scheduler())
        ARIADNE_TEST_FAIL(Logger::instance().use_blocking_scheduler())
        ARIADNE_TEST_FAIL(Logger::instance().use_nonblocking_scheduler())
        ARIADNE_TEST_FAIL(Logger::instance().register_thread(std::this_thread::get_id(),"no-registry"))
        ARIADNE_TEST_FAIL(Logger::instance().register_self_thread("no-registry",1))
        ARIADNE_TEST_FAIL(Logger::instance().unregister_thread(std::this_thread::get_id()))
        Logger::instance().attach_thread_registry(&_registry);
        ARIADNE_TEST_FAIL(Logger::instance().attach_thread_registry(&_registry))
        ARIADNE_TEST_EXECUTE(Logger::instance().use_immediate_scheduler())
        ARIADNE_TEST_EXECUTE(Logger::instance().use_blocking_scheduler())
        ARIADNE_TEST_EXECUTE(Logger::instance().use_nonblocking_scheduler())
        _registry.set_threads_registered(1);
        ARIADNE_TEST_FAIL(Logger::instance().use_immediate_scheduler())
        ARIADNE_TEST_FAIL(Logger::instance().use_blocking_scheduler())
        ARIADNE_TEST_FAIL(Logger::instance().use_nonblocking_scheduler())
        _registry.set_threads_registered(0);
    }

    void test_print_configuration() {
        Logger::instance().use_immediate_scheduler();
        Logger::instance().configuration().set_verbosity(1);
        LOGGING_PRINTLN(Logger::instance().configuration())

        std::ostringstream invalid_policy;
        invalid_policy << static_cast<ThreadNamePrintingPolicy>(255);
        ARIADNE_TEST_EQUALS(invalid_policy.str().compare("NEVER"), 0);
    }

    void test_style_branch_combinations() {
        ARIADNE_TEST_ASSERT(!TT_STYLE_NONE.is_styled());
        ARIADNE_TEST_ASSERT(TerminalTextStyle(1,0,false,false).is_styled());
        ARIADNE_TEST_ASSERT(TerminalTextStyle(0,1,false,false).is_styled());
        ARIADNE_TEST_ASSERT(TerminalTextStyle(0,0,true,false).is_styled());
        ARIADNE_TEST_ASSERT(TerminalTextStyle(0,0,false,true).is_styled());

        using ThemeField = TerminalTextStyle TerminalTextTheme::*;
        const std::array<ThemeField,14> fields = {
            &TerminalTextTheme::level_number,
            &TerminalTextTheme::level_shown_separator,
            &TerminalTextTheme::level_hidden_separator,
            &TerminalTextTheme::multiline_separator,
            &TerminalTextTheme::assignment_comparison,
            &TerminalTextTheme::miscellaneous_operator,
            &TerminalTextTheme::round_parentheses,
            &TerminalTextTheme::square_parentheses,
            &TerminalTextTheme::curly_parentheses,
            &TerminalTextTheme::colon,
            &TerminalTextTheme::comma,
            &TerminalTextTheme::number,
            &TerminalTextTheme::at,
            &TerminalTextTheme::keyword
        };

        ARIADNE_TEST_ASSERT(!TT_THEME_NONE.has_style());
        for (auto field : fields) {
            TerminalTextTheme theme;
            theme.*field = TerminalTextStyle(1,0,false,false);
            ARIADNE_TEST_ASSERT(theme.has_style());
        }
    }

    void test_parser_branch_boundaries() {
        Logger::instance().use_immediate_scheduler();
        Logger::instance().configuration().set_verbosity(1);
        Logger::instance().configuration().set_theme(TT_THEME_DARK);
        Logger::instance().configuration().add_custom_keyword("edge");

        // Exercise operator cases and parser boundaries that are otherwise
        // semantically equivalent but distinct branches in llvm-cov.
        LOGGING_PRINTLN("! / \\ | & %")
        LOGGING_PRINTLN(".1 a. a1 11a 111")
        LOGGING_PRINTLN("edge edgeA Aedge [edge]")

        // Use a theme that styles keywords only, so the characters immediately
        // preceding "edge" reach isalphanumeric_withstylecodes() unchanged.
        TerminalTextTheme keyword_only_theme;
        keyword_only_theme.keyword = TT_STYLE_DARKORANGE;
        Logger::instance().configuration().set_theme(keyword_only_theme);

        // c == 'm' with pos <= 3.
        LOGGING_PRINTLN("medge")
        // Non-alphanumeric predecessor: !isalpha(c) is true and isdigit(c) is false.
        LOGGING_PRINTLN("[edge]")
        // Reset-code path with an alphabetic character immediately before ESC[0m.
        LOGGING_PRINTLN("A" << TerminalTextStyle::RESET << "edge")
        // With keyword-only styling, this raw reset sequence reaches the
        // keyword parser unchanged and makes the pre-reset character alphabetic.
        LOGGING_PRINTLN(std::string("A\033[0medge"))
    }

    void test_stylecode_adjacency_helper() {
        ARIADNE_TEST_ASSERT(!isalphanumeric_withstylecodes("[",0));
        ARIADNE_TEST_ASSERT(isalphanumeric_withstylecodes("A",0));
        ARIADNE_TEST_ASSERT(isalphanumeric_withstylecodes("1",0));
        ARIADNE_TEST_ASSERT(isalphanumeric_withstylecodes("m",0));

        std::string alpha_reset = std::string("A\033[0m");
        std::string digit_reset = std::string("1\033[0m");
        std::string punctuation_reset = std::string("[\033[0m");

        ARIADNE_TEST_ASSERT(isalphanumeric_withstylecodes(alpha_reset,alpha_reset.size()-1));
        ARIADNE_TEST_ASSERT(isalphanumeric_withstylecodes(digit_reset,digit_reset.size()-1));
        ARIADNE_TEST_ASSERT(!isalphanumeric_withstylecodes(punctuation_reset,punctuation_reset.size()-1));
    }

    void test_scheduler_noop_registration_paths() {
        Logger::instance().use_immediate_scheduler();
        // Registration is intentionally a no-op for the immediate scheduler.
        Logger::instance().register_thread(std::this_thread::get_id(),"ignored");
        Logger::instance().register_self_thread("ignored",1);
        Logger::instance().unregister_thread(std::this_thread::get_id());

        Logger::instance().use_blocking_scheduler();
        // Unknown ids exercise the not-found branches without changing state.
        Logger::instance().unregister_thread(std::thread::id());

        Logger::instance().use_nonblocking_scheduler();
        Logger::instance().unregister_thread(std::thread::id());
        Logger::instance().use_blocking_scheduler();
    }

    void test_hold_release_missing_scope() {
        Logger::instance().use_immediate_scheduler();
        Logger::instance().configuration().set_verbosity(2);
        Logger::instance().hold("coverage-existing","held");
        Logger::instance().release("coverage-missing");
        Logger::instance().release("coverage-existing");
        // Release while nothing is held.
        Logger::instance().release("coverage-empty");
    }

    void test_remaining_branch_boundaries() {
        Logger::instance().use_immediate_scheduler();
        Logger::instance().configuration().set_verbosity(20);
        Logger::instance().configuration().set_theme(TT_THEME_DARK);

        // Empty text makes the RHS of "handles_multiline && text.size()>0"
        // false while multiline handling itself remains enabled.
        Logger::instance().configuration().set_handles_multiline_output(true);
        LOGGING_PRINTLN("")

        // Number parser: digit at begin(), and a second digit whose predecessor
        // is begin(), cover the two iterator boundary branches.
        LOGGING_PRINTLN("1 12")

        // Keyword boundary at end of the string makes kw_pos+length == size.
        LOGGING_PRINTLN("edge")

        // Exercise styled-keyword adjacency where the preceding character is
        // alphabetic before an ANSI reset sequence.
        LOGGING_PRINTLN("A" << TT_STYLE_DARKORANGE() << "x" << TerminalTextStyle::RESET << "edge")

        // Extraline preamble with indentation disabled.
        Logger::instance().configuration().set_indents_based_on_level(false);
        LOGGING_PRINTLN("first\nsecond")
        Logger::instance().configuration().set_indents_based_on_level(true);

        // For a level > 9, same level and same thread with level-on-change,
        // exercise the hidden two-column level branch.
        Logger::instance().configuration().set_prints_level_on_change_only(true);
        Logger::instance().increase_level(9);
        LOGGING_PRINTLN("same-high-level")
        LOGGING_PRINTLN("same-high-level-again")
        Logger::instance().decrease_level(9);
        Logger::instance().configuration().set_prints_level_on_change_only(false);

        // Exactly max_columns+1 held columns: level 1 contributes four columns,
        // therefore 77 text characters reach the second equality condition
        // for the default 80-column non-TTY test environment.
        Logger::instance().configuration().set_theme(TT_THEME_NONE);
        Logger::instance().hold("coverage-exact-held",std::string(77,'x'));
        Logger::instance().release("coverage-exact-held");

        // redirect_to_console() when no redirect file is open.
        Logger::instance().redirect_to_console();
    }

    void test_window_columns() {
#ifndef _WIN32
        int master_fd = posix_openpt(O_RDWR);
        ARIADNE_TEST_ASSERT(master_fd >= 0);
        if (master_fd < 0) return;
        ARIADNE_TEST_EQUALS(grantpt(master_fd),0);
        ARIADNE_TEST_EQUALS(unlockpt(master_fd),0);
        char* slave_name = ptsname(master_fd);
        ARIADNE_TEST_ASSERT(slave_name != nullptr);
        if (slave_name == nullptr) { close(master_fd); return; }

        int slave_fd = open(slave_name,O_RDWR);
        ARIADNE_TEST_ASSERT(slave_fd >= 0);
        if (slave_fd < 0) { close(master_fd); return; }

        int saved_stdout = dup(STDOUT_FILENO);
        ARIADNE_TEST_ASSERT(saved_stdout >= 0);
        if (saved_stdout < 0) { close(slave_fd); close(master_fd); return; }

        struct winsize ws {};
        ws.ws_col = 123;
        ARIADNE_TEST_EQUALS(ioctl(slave_fd,TIOCSWINSZ,&ws),0);
        ARIADNE_TEST_EQUALS(dup2(slave_fd,STDOUT_FILENO),STDOUT_FILENO);
        ARIADNE_TEST_EQUALS(Logger::instance().get_window_columns(),123u);

        ws.ws_col = 600;
        ARIADNE_TEST_EQUALS(ioctl(slave_fd,TIOCSWINSZ,&ws),0);
        ARIADNE_TEST_EQUALS(Logger::instance().get_window_columns(),80u);

        ARIADNE_TEST_EQUALS(dup2(saved_stdout,STDOUT_FILENO),STDOUT_FILENO);
        close(saved_stdout);
        close(slave_fd);
        close(master_fd);
#else
        ARIADNE_TEST_EQUALS(Logger::instance().get_window_columns(),80u);
#endif
    }

    void test_shown_single_print() {
        Logger::instance().use_immediate_scheduler();
        Logger::instance().configuration().set_verbosity(1);
        LOGGING_PRINTLN("This is a call on level 1")
    }

    void test_hidden_single_print() {
        Logger::instance().use_immediate_scheduler();
        Logger::instance().configuration().set_verbosity(0);
        LOGGING_PRINTLN("This is a hidden call on level 1")
    }

    void test_muted_print() {
        Logger::instance().use_immediate_scheduler();
        Logger::instance().configuration().set_verbosity(1);
        LOGGING_PRINTLN("This is the first line shown.")
        LOGGING_RUN_MUTED(print_something1())
        LOGGING_PRINTLN("This is the second and last line shown.")
    }

    void test_use_blocking_scheduler() {
        Logger::instance().use_blocking_scheduler();
        Logger::instance().configuration().set_verbosity(1);
        LOGGING_PRINTLN("This is a call")
        LOGGING_PRINTLN("This is another call")
    }

    void test_use_nonblocking_scheduler() {
        Logger::instance().use_nonblocking_scheduler();
        Logger::instance().configuration().set_verbosity(1);
        LOGGING_PRINTLN("This is a call")
        LOGGING_PRINTLN("This is another call")
    }

    void test_shown_call_function_with_entrance_and_exit() {
        Logger::instance().use_immediate_scheduler();
        Logger::instance().configuration().set_verbosity(2);
        Logger::instance().configuration().set_prints_scope_entrance(true);
        Logger::instance().configuration().set_prints_scope_exit(true);
        LOGGING_PRINTLN("This is a call on level 1");
        LOGGING_RUN_AT(0,sample_function());
        LOGGING_PRINTLN("This is again a call on level 1");
    }

    void test_hide_call_function_with_entrance_and_exit() {
        Logger::instance().use_immediate_scheduler();
        Logger::instance().configuration().set_verbosity(2);
        Logger::instance().configuration().set_prints_scope_entrance(false);
        Logger::instance().configuration().set_prints_scope_exit(false);
        LOGGING_PRINTLN("This is a call on level 1");
        LOGGING_RUN_AT(1,sample_function());
        LOGGING_PRINTLN("This is again a call on level 1");
    }

    void test_indents_based_on_level() {
        Logger::instance().use_immediate_scheduler();
        Logger::instance().configuration().set_verbosity(2);
        Logger::instance().configuration().set_indents_based_on_level(true);
        LOGGING_PRINTLN("Call at level 1");
        LOGGING_PRINTLN_AT(1,"Call at level 2");
        Logger::instance().configuration().set_indents_based_on_level(false);
        LOGGING_PRINTLN("Call at level 1");
        LOGGING_PRINTLN_AT(1,"Call at level 2");
    }

    void test_high_level_multiline_hold() {
        Logger::instance().use_blocking_scheduler();
        Logger::instance().configuration().set_theme(TT_THEME_NONE);
        Logger::instance().configuration().set_verbosity(20);
        Logger::instance().configuration().set_handles_multiline_output(true);
        Logger::instance().configuration().set_indents_based_on_level(true);
        Logger::instance().configuration().set_thread_name_printing_policy(ThreadNamePrintingPolicy::BEFORE);

        Logger::instance().increase_level(9);
        Logger::instance().hold("coverage-high-level","held");
        LOGGING_PRINTLN("first line\nsecond line")
        SizeType num_cols = Logger::instance().get_window_columns();
        LOGGING_PRINTLN(std::string(num_cols+10,'x') << "\nlast")
        Logger::instance().release("coverage-high-level");
        Logger::instance().decrease_level(9);

        Logger::instance().configuration().set_thread_name_printing_policy(ThreadNamePrintingPolicy::NEVER);
    }

    void test_high_level_hidden_level_and_hold_reprint() {
        Logger::instance().use_blocking_scheduler();
        Logger::instance().configuration().set_theme(TT_THEME_NONE);
        Logger::instance().configuration().set_verbosity(20);
        Logger::instance().configuration().set_handles_multiline_output(true);
        Logger::instance().configuration().set_prints_level_on_change_only(true);
        Logger::instance().configuration().set_thread_name_printing_policy(ThreadNamePrintingPolicy::BEFORE);

        Logger::instance().increase_level(9);
        Logger::instance().hold("coverage-hidden-level","held-value");

        // First print establishes cached level 10. The second print keeps the
        // same level, so the two-character hidden-level branch is exercised.
        LOGGING_PRINTLN("level-ten-first")
        LOGGING_PRINTLN("level-ten-second")

        // Keep the hold active across a completed println so the held line is
        // deterministically reprinted at the end of _println().
        LOGGING_PRINTLN("held-line-reprint")

        Logger::instance().release("coverage-hidden-level");
        Logger::instance().decrease_level(9);

        Logger::instance().configuration().set_prints_level_on_change_only(false);
        Logger::instance().configuration().set_thread_name_printing_policy(ThreadNamePrintingPolicy::NEVER);

        Logger::instance().configuration().set_handles_multiline_output(false);
        Logger::instance().hold("coverage-final-reprint","held-final");
        LOGGING_PRINTLN("single-line while held")
        Logger::instance().release("coverage-final-reprint");
        Logger::instance().configuration().set_handles_multiline_output(true);
    }

    void test_handles_multiline_output() {
        SizeType num_cols = Logger::instance().get_window_columns();
        ARIADNE_PRINT_TEST_COMMENT("Number of window columns: " << num_cols)
        Logger::instance().use_immediate_scheduler();
        Logger::instance().configuration().set_verbosity(2);
        Logger::instance().configuration().set_handles_multiline_output(true);
        LOGGING_PRINTLN("<begin>" << std::string(num_cols,' ') << "<end>")
        Logger::instance().configuration().set_handles_multiline_output(false);
        LOGGING_PRINTLN("<begin>" << std::string(num_cols,' ') << "<end>")
    }

    void test_discards_newlines_and_indentation() {
        Logger::instance().use_immediate_scheduler();
        Logger::instance().configuration().set_verbosity(2);
        Logger::instance().configuration().set_discards_newlines_and_indentation(true);
        LOGGING_PRINTLN("This text should just be in a single line \n       with no extra whitespaces.");
        Logger::instance().configuration().set_discards_newlines_and_indentation(false);
        LOGGING_PRINTLN("This text should be in two lines\n          where the second one starts several whitespaces in.");
    }

    void _hold_short_line() {
        LOGGING_SCOPE_CREATE;
        ProgressIndicator indicator(10.0);
        for (unsigned int i=0; i<10; ++i) {
            indicator.update_current(i);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            LOGGING_SCOPE_PRINTHOLD("[" << indicator.symbol() << "] " << indicator.percentage() << "%");
        }
    }

    void test_hold_line() {
        Logger::instance().configuration().set_verbosity(2);
        Logger::instance().use_immediate_scheduler();
        _hold_short_line();
        Logger::instance().use_blocking_scheduler();
        _hold_short_line();
        Logger::instance().use_nonblocking_scheduler();
        _hold_short_line();
    }

    void test_hold_line_with_newline_println() {
        Logger::instance().use_immediate_scheduler();
        LOGGING_SCOPE_CREATE;
        Logger::instance().configuration().set_verbosity(2);
        ProgressIndicator indicator(10.0);
        for (unsigned int i=0; i<10; ++i) {
            indicator.update_current(i);
            LOGGING_PRINTLN("first line\nsecond line")
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            LOGGING_SCOPE_PRINTHOLD("[" << indicator.symbol() << "] " << indicator.percentage() << "%");
        }
    }

    void test_hold_long_line() {
        Logger::instance().use_immediate_scheduler();
        Logger::instance().configuration().set_verbosity(2);
        LOGGING_SCOPE_CREATE;
        SizeType num_cols = Logger::instance().get_window_columns();

        std::string exactly_str(num_cols-4,'x'); // exactly the length required to fill the columns (given a prefix of 4 chars)
        std::string larger_str(num_cols,'x'); // larger enough

        for (unsigned int i=0; i<10; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            LOGGING_PRINTLN("i="<<i)
            LOGGING_SCOPE_PRINTHOLD(exactly_str);
        }

        for (unsigned int i=0; i<10; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            LOGGING_PRINTLN("i="<<i)
            LOGGING_SCOPE_PRINTHOLD(larger_str);
        }
    }

    void test_hold_multiple() {
        Logger::instance().use_immediate_scheduler();
        Logger::instance().configuration().set_verbosity(4);
        LOGGING_SCOPE_CREATE;
        SizeType u=30;
        for (unsigned int i=0; i<3; ++i) {
            sample_printhold_nested_loop("intermediate",u);
            std::this_thread::sleep_for(std::chrono::milliseconds(u));
            LOGGING_SCOPE_PRINTHOLD("external@"<<i);
        }
    }

    void test_light_theme() {
        Logger::instance().use_immediate_scheduler();
        Logger::instance().configuration().set_verbosity(2);
        Logger::instance().configuration().set_theme(TT_THEME_LIGHT);
        std::clog << TT_THEME_LIGHT << std::endl;
        LOGGING_PRINTLN("This is a call on level 1")
        LOGGING_RUN_AT(0,sample_function())
        LOGGING_PRINTLN("This is again a call on level 1")
        Logger::instance().configuration().set_theme(TT_THEME_NONE);
    }

    void test_dark_theme() {
        Logger::instance().use_immediate_scheduler();
        Logger::instance().configuration().set_verbosity(2);
        Logger::instance().configuration().set_theme(TT_THEME_DARK);
        std::clog << TT_THEME_DARK << std::endl;
        LOGGING_PRINTLN("This is a call on level 1")
        LOGGING_RUN_AT(0,sample_function())
        LOGGING_PRINTLN("This is again a call on level 1 + 2 - 3")
    }

    void test_theme_custom_keyword() {
        Logger::instance().use_immediate_scheduler();
        Logger::instance().configuration().set_verbosity(1);
        Logger::instance().configuration().set_theme(TT_THEME_DARK);
        Logger::instance().configuration().add_custom_keyword("first");
        Logger::instance().configuration().add_custom_keyword("second", TT_STYLE_DARKORANGE);
        LOGGING_PRINTLN("This is a default first, a styled second, an ignored secondsecond and msecond and second1 and 1second and firstsecond but not ignored [second and second]")
    }

    void test_theme_bgcolor_bold_underline() {
        Logger::instance().use_immediate_scheduler();
        Logger::instance().configuration().set_theme(TT_THEME_DARK);
        std::list<TerminalTextStyle> styles;
        styles.push_back(TerminalTextStyle(0,0,true,false));
        styles.push_back(TerminalTextStyle(0,0,false,true));
        styles.push_back(TerminalTextStyle(0,0,true,true));
        styles.push_back(TerminalTextStyle(0,88,false,false));
        styles.push_back(TerminalTextStyle(0,88,true,false));
        styles.push_back(TerminalTextStyle(0,88,false,true));
        styles.push_back(TerminalTextStyle(0,88,true,true));
        std::ostringstream ss;
        for (auto style: styles) {
            ss << style() << "x" << TerminalTextStyle::RESET << " ";
        }
        std::clog << ss.str() << std::endl;
    }

    void test_redirect() {
        Logger::instance().use_immediate_scheduler();
        Logger::instance().configuration().set_verbosity(1);
        Logger::instance().configuration().set_theme(TT_THEME_DARK);
        LOGGING_PRINTLN("This is call 1");
        Logger::instance().redirect_to_file("log.txt");
        Logger::instance().redirect_to_file("log.txt");
        LOGGING_PRINTLN("This is call 2");
        LOGGING_PRINTLN("This is call 3");
        Logger::instance().redirect_to_console();
        LOGGING_PRINTLN("This is call 4");
        LOGGING_PRINTLN("This is call 5");

        std::string line;
        std::ifstream file("log.txt");
        unsigned int count = 0;
        if(file.is_open()) {
            while(!file.eof()) {
                getline(file,line);
                count++;
            }
            file.close();
        }
        ARIADNE_TEST_EQUALS(count,3);
    }

    void test_multiple_threads_with_blocking_scheduler() {
        Logger::instance().use_blocking_scheduler();
        Logger::instance().configuration().set_verbosity(3);
        Logger::instance().configuration().set_theme(TT_THEME_DARK);
        Logger::instance().configuration().set_thread_name_printing_policy(ThreadNamePrintingPolicy::BEFORE);
        LOGGING_PRINTLN("Printing on the " << Logger::instance().current_thread_name() << " thread without other threads");
        ARIADNE_TEST_EQUALS(Logger::instance().cached_last_printed_thread_name().compare("main"), 0);

        std::string thread1_name;
        std::string thread2_name;
        {
            Thread thread1([&thread1_name] {
                thread1_name = Logger::instance().current_thread_name();
                print_something1();
            },"thr1");
            Thread thread2([&thread2_name] {
                thread2_name = Logger::instance().current_thread_name();
                print_something2();
            },"thr2");
            LOGGING_PRINTLN("Printing again on the main thread, but with other threads");
        }

        ARIADNE_TEST_EQUALS(thread1_name.compare("thr1"), 0);
        ARIADNE_TEST_EQUALS(thread2_name.compare("thr2"), 0);
    }

    void test_multiple_threads_with_nonblocking_scheduler() {
        Logger::instance().use_nonblocking_scheduler();
        Logger::instance().configuration().set_theme(TT_THEME_DARK);
        Logger::instance().configuration().set_verbosity(3);
        Logger::instance().configuration().set_thread_name_printing_policy(ThreadNamePrintingPolicy::BEFORE);

        LOGGING_PRINTLN("Printing on the " << Logger::instance().current_thread_name() << " thread without other threads");
        Thread thread1([] { print_something1(); });
        Thread thread2([] { print_something1(); });
        Thread thread3([] { print_something1(); });
        Thread thread4([] { print_something1(); });
        Thread thread5([] { print_something1(); });
        Thread thread6([] { print_something1(); });
        LOGGING_PRINTLN("Printing again on the main thread, but with other threads");
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    void test_register_self_thread() {
        Logger::instance().use_blocking_scheduler();
        Logger::instance().configuration().set_verbosity(3);
        Logger::instance().configuration().set_theme(TT_THEME_DARK);
        Logger::instance().configuration().set_thread_name_printing_policy(ThreadNamePrintingPolicy::BEFORE);
        LOGGING_PRINTLN("Printing on the " << Logger::instance().current_thread_name() << " thread without other threads");
        ARIADNE_TEST_EQUALS(Logger::instance().cached_last_printed_thread_name().compare("main"), 0);
        std::thread::id thread_id;
        std::thread thread1([&thread_id] { thread_id = std::this_thread::get_id(); Logger::instance().register_self_thread("thr1", 1); print_something1(); });
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        thread1.join();
        Logger::instance().unregister_thread(thread_id);
    }

    void test_printing_policy_with_theme_and_print_level(bool use_theme, bool print_level) {
        ARIADNE_PRINT_TEST_COMMENT("Policies: " << ThreadNamePrintingPolicy::BEFORE << " " << ThreadNamePrintingPolicy::AFTER << " " << ThreadNamePrintingPolicy::NEVER)
        Logger::instance().use_immediate_scheduler();
        Logger::instance().use_blocking_scheduler();
        Logger::instance().configuration().set_verbosity(3);
        if (use_theme) Logger::instance().configuration().set_theme(TT_THEME_DARK);
        else Logger::instance().configuration().set_theme(TT_THEME_NONE);
        Logger::instance().configuration().set_prints_level_on_change_only(print_level);
        Logger::instance().configuration().set_thread_name_printing_policy(ThreadNamePrintingPolicy::BEFORE);
        Thread thread1([] { print_something1(); },"thr1");
        LOGGING_PRINTLN("Printing thread name before");
        LOGGING_PRINTLN("Printing thread name before again");
        Logger::instance().configuration().set_thread_name_printing_policy(ThreadNamePrintingPolicy::AFTER);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        Thread thread2([] { print_something1(); },"thr2");
        LOGGING_PRINTLN("Printing thread name after");
        LOGGING_PRINTLN("Printing thread name after again");
        Logger::instance().configuration().set_thread_name_printing_policy(ThreadNamePrintingPolicy::NEVER);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        LOGGING_PRINTLN("Printing thread name never");
        Thread thread3([] { print_something1(); },"thr3");
    }
};

int main() {

    TestLogging().test();

    return ARIADNE_TEST_FAILURES;
}

