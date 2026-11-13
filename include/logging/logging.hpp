/***************************************************************************
 *            logging/logging.hpp
 *
 *  Copyright  2021  Luca Geretti
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

/** @file logging/logging.hpp
 *  @brief Concurrent logging, message scheduling, and logger configuration.
 */

#ifndef ARIADNE_LOGGING_LOGGING_HPP
#define ARIADNE_LOGGING_LOGGING_HPP

#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <shared_mutex>
#include <sstream>
#include <thread>
#include <utility>
#include <vector>

#include "logging/thread_registry_interface.hpp"

#if defined(linux) || defined(__linux) || defined(__linux__)
#define LOGGING_PRETTY_FUNCTION __PRETTY_FUNCTION__
#elif defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
#define LOGGING_PRETTY_FUNCTION __FUNCSIG__
#elif defined(darwin) || defined(__darwin) || defined(__darwin__) || defined(__APPLE__)
#define LOGGING_PRETTY_FUNCTION __PRETTY_FUNCTION__
#else
#define LOGGING_PRETTY_FUNCTION ""
#endif

// Automatic level increase/decrease in a scope; meant to be used once within a function, at top scope; necessary for print holding.
#define LOGGING_SCOPE_CREATE auto logscopemanager = LogScopeManager(LOGGING_PRETTY_FUNCTION);
// Managed level increase/decrease around the function fn; if the function throws, manual decrease of the proper level is required.
#define LOGGING_RUN_AT(level, fn)             \
    Logger::instance().increase_level(level); \
    fn;                                       \
    Logger::instance().decrease_level(level);
// Mute the logger for the function fn; if the function throws, manual decrease of the proper level is required.
#define LOGGING_RUN_MUTED(fn)                 \
    Logger::instance().mute_increase_level(); \
    fn;                                       \
    Logger::instance().mute_decrease_level();
// Print one line at the current level; the text shouldn't have carriage returns, but for efficiency purposes this is not checked.
#define LOGGING_PRINTLN(text)                                   \
    {                                                           \
        if (!Logger::instance().is_muted_at(0)) {               \
            std::ostringstream logger_stream;                   \
            logger_stream << std::boolalpha << text;            \
            Logger::instance().println(0, logger_stream.str()); \
        }                                                       \
    }
// Print one line at an increased level with respect to the current one; the text shouldn't have carriage returns, but for efficiency purposes this is not checked.
#define LOGGING_PRINTLN_AT(level, text)                             \
    {                                                               \
        if (!Logger::instance().is_muted_at(level)) {               \
            std::ostringstream logger_stream;                       \
            logger_stream << std::boolalpha << text;                \
            Logger::instance().println(level, logger_stream.str()); \
        }                                                           \
    }
// Print variable in one line at the current level, using the formatting convention.
#define LOGGING_PRINTLN_VAR(var)                                     \
    {                                                                \
        if (!Logger::instance().is_muted_at(0)) {                    \
            std::ostringstream logger_stream;                        \
            logger_stream << std::boolalpha << #var << " = " << var; \
            Logger::instance().println(0, logger_stream.str());      \
        }                                                            \
    }
// Print variable in one line at the increased level with respect to the current one, using the formatting convention.
#define LOGGING_PRINTLN_VAR_AT(level, var)                           \
    {                                                                \
        if (!Logger::instance().is_muted_at(level)) {                \
            std::ostringstream logger_stream;                        \
            logger_stream << std::boolalpha << #var << " = " << var; \
            Logger::instance().println(level, logger_stream.str());  \
        }                                                            \
    }
// Print a text at the bottom line, holding it until the function scope ends; this requires creation of the scope.
// Nested calls in separate functions append to the held line.
// The text for obvious reasons shouldn't have newlines and carriage returns; for efficiency purposes this is not checked.
#define LOGGING_SCOPE_PRINTHOLD(text)                                              \
    {                                                                              \
        if (!Logger::instance().is_muted_at(0)) {                                  \
            std::ostringstream logger_stream;                                      \
            logger_stream << std::boolalpha << text;                               \
            Logger::instance().hold(LOGGING_PRETTY_FUNCTION, logger_stream.str()); \
        }                                                                          \
    }

namespace Ariadne {

using OutputStream = std::ostream;
template<class T>
using SharedPointer = std::shared_ptr<T>;
using SizeType = size_t;

//! \brief Exception for trying to use threaded functionality with no attached thread registry
class LoggerNoThreadRegistryException : public std::exception {};
//! \brief Exception for trying to modify the thread registry, which should be immutable as soon as attached
class LoggerModifyThreadRegistryException : public std::exception {};
//! \brief Exception for trying to change the scheduler while there are registered threads
class LoggerSchedulerChangeWithRegisteredThreadsException : public std::exception {};

//! \brief A styling for a text character
//! \details Refer to https://www.lihaoyi.com/post/BuildyourownCommandLinewithANSIescapecodes.html#256-colors
//! for the detailed 256 map of colors for text and background
struct TerminalTextStyle {
    /** @brief Construct a \c TerminalTextStyle from fontcolor, bgcolor, bold, underline. */
    TerminalTextStyle(uint8_t fontcolor, uint8_t bgcolor, bool bold, bool underline);

    //! \brief Return the string code that applies the style
    std::string operator()() const;

    //! \brief If a style is actually set
    bool is_styled() const;

    uint8_t fontcolor; // The color of the character
    uint8_t bgcolor;   // The color of the background of the character box
    bool bold;         // If the character must be highlighted in bold
    bool underline;    // If the character must be underlined

    inline static std::string RESET = "\u001b[0m";
};

static TerminalTextStyle TT_STYLE_NONE(0, 0, false, false);
static TerminalTextStyle TT_STYLE_DARKBROWN(95, 0, false, false);
static TerminalTextStyle TT_STYLE_DARKORANGE(130, 0, false, false);
static TerminalTextStyle TT_STYLE_LIGHTBROWN(136, 0, false, false);
static TerminalTextStyle TT_STYLE_BRIGHTORANGE(208, 0, false, false);
static TerminalTextStyle TT_STYLE_CREAM(223, 0, false, false);
static TerminalTextStyle TT_STYLE_OBSIDIAN(237, 0, false, false);
static TerminalTextStyle TT_STYLE_DARKGREY(244, 0, false, false);
static TerminalTextStyle TT_STYLE_LIGHTGREY(251, 0, false, false);

//! \brief A set of text styles for different elements of the logging interface
struct TerminalTextTheme {
    /** @brief Construct a \c TerminalTextTheme with the specified characteristics. */
    TerminalTextTheme();
    /** @brief Construct a \c TerminalTextTheme from level_number, level_shown_separator, level_hidden_separator, multiline_separator, assignment_comparison, miscellaneous_operator, round_parentheses, square_parentheses, curly_parentheses, colon, comma, number, at, keyword. */
    TerminalTextTheme(TerminalTextStyle level_number, TerminalTextStyle level_shown_separator, TerminalTextStyle level_hidden_separator,
                      TerminalTextStyle multiline_separator, TerminalTextStyle assignment_comparison, TerminalTextStyle miscellaneous_operator,
                      TerminalTextStyle round_parentheses, TerminalTextStyle square_parentheses, TerminalTextStyle curly_parentheses,
                      TerminalTextStyle colon, TerminalTextStyle comma, TerminalTextStyle number, TerminalTextStyle at, TerminalTextStyle keyword);

    TerminalTextStyle level_number;           // The number representing the log level
    TerminalTextStyle level_shown_separator;  // The separator when the level is shown
    TerminalTextStyle level_hidden_separator; // The separator when the level is hidden

    TerminalTextStyle multiline_separator;    // The separator for multiple lines output
    TerminalTextStyle assignment_comparison;  // Characters: =, >, <, !
    TerminalTextStyle miscellaneous_operator; // Characters: +, -, *, /, \, ^, |, &, %

    TerminalTextStyle round_parentheses;  // Characters: ( )
    TerminalTextStyle square_parentheses; // Characters: [ ]
    TerminalTextStyle curly_parentheses;  // Characters: { }

    TerminalTextStyle colon;  // Character: :
    TerminalTextStyle comma;  // Character: ,
    TerminalTextStyle number; // A 0-9 digit, or a dot after a digit, but not if parsing a numbered_variable

    TerminalTextStyle at;      // Character: @
    TerminalTextStyle keyword; // Specific words, not pre/post-fixed by an alphanumeric character: virtual, const, true, false, inf

    /** @brief Test whether at least one terminal style is enabled. */
    bool has_style() const; // Whether at least one style is set

    /** @brief Write the value to the output stream. */
    friend OutputStream& operator<<(OutputStream& os, const TerminalTextTheme& theme);
};

//! \brief Empty theme, for not forcing any theme
static TerminalTextTheme TT_THEME_NONE = TerminalTextTheme();
//! \brief Theme for black background
static TerminalTextTheme TT_THEME_DARK = TerminalTextTheme(TT_STYLE_BRIGHTORANGE, TT_STYLE_BRIGHTORANGE, TT_STYLE_DARKGREY,
                                                           TT_STYLE_DARKGREY, TT_STYLE_BRIGHTORANGE, TT_STYLE_LIGHTBROWN,
                                                           TT_STYLE_DARKORANGE, TT_STYLE_DARKGREY, TT_STYLE_LIGHTBROWN,
                                                           TT_STYLE_LIGHTBROWN, TT_STYLE_DARKGREY, TT_STYLE_CREAM,
                                                           TT_STYLE_DARKGREY, TT_STYLE_DARKGREY);
//! \brief Theme for white background
static TerminalTextTheme TT_THEME_LIGHT = TerminalTextTheme(TT_STYLE_BRIGHTORANGE, TT_STYLE_BRIGHTORANGE, TT_STYLE_DARKGREY,
                                                            TT_STYLE_DARKGREY, TT_STYLE_DARKBROWN, TT_STYLE_DARKBROWN,
                                                            TT_STYLE_DARKGREY, TT_STYLE_OBSIDIAN, TT_STYLE_LIGHTBROWN,
                                                            TT_STYLE_LIGHTBROWN, TT_STYLE_OBSIDIAN, TT_STYLE_DARKORANGE,
                                                            TT_STYLE_DARKGREY, TT_STYLE_DARKGREY);

//! \brief Support class for managing log level increase/decrease in a scope
//! \details Since it is possible to capture the scope string of a function only, nested scopes in a function have the same scope string
//! (but work as expected in terms of level management)
class LogScopeManager {
  public:
    //! \brief Construct with a given scope, and optionally choosing the amount of level increase
    LogScopeManager(std::string scope, unsigned int level_increase = 1);
    /** @brief Return the log-scope identifier. */
    std::string scope() const;

  public:
    /** @brief Release held scope output and restore the previous log level. */
    virtual ~LogScopeManager();

  private:
    const std::string _scope;
    const unsigned int _level_increase;
};

//! \brief Supported kinds of log messages
enum RawMessageKind { PRINTLN,
                      HOLD,
                      RELEASE };

//! \brief A log message in raw form, as submitted to a concurrent scheduler
//! \details This does not hold any thread identifier information yet, for efficiency
struct LogThinRawMessage {
    /** @brief Construct a \c LogThinRawMessage from scope, level, text. */
    LogThinRawMessage(std::string scope, unsigned int level, std::string text);

    std::string scope;
    unsigned int level;
    std::string text;

    /** @brief Return the category discriminator. */
    RawMessageKind kind() const;
};

//! \brief Full log message information in raw form, before formatting for actual output
struct LogRawMessage : public LogThinRawMessage {
    /** @brief Construct a \c LogRawMessage from id, msg. */
    LogRawMessage(std::string id, LogThinRawMessage msg) : LogThinRawMessage(msg), identifier(id) {}
    /** @brief Construct a \c LogRawMessage from id, scp, lvl, txt. */
    LogRawMessage(std::string id, std::string scp, unsigned int lvl, std::string txt) : LogRawMessage(id, LogThinRawMessage(scp, lvl, txt)) {}
    /** @brief Construct a \c LogRawMessage from scp, lvl, txt. */
    LogRawMessage(std::string scp, unsigned int lvl, std::string txt) : LogThinRawMessage(scp, lvl, txt), identifier("") {}
    std::string identifier;
};

class LoggerData;

//! \brief The policy for printing the thread id with respect to the log level: NEVER, BEFORE or AFTER
enum class ThreadNamePrintingPolicy { NEVER,
                                      BEFORE,
                                      AFTER };

/** @brief Write the value to the output stream. */
OutputStream& operator<<(OutputStream& os, const ThreadNamePrintingPolicy& p);

//! \brief Return whether the character immediately preceding a pos is alphanumeric,
//! accounting for an ANSI reset sequence ending at a pos.
bool isalphanumeric_withstylecodes(const std::string& text, size_t pos);

//! \brief Configuration of visualisation settings for a Logger
class LoggerConfiguration {
  public:
    /** @brief Construct a \c LoggerConfiguration with the specified characteristics. */
    LoggerConfiguration();

    //! \brief Configuration setters

    //! \brief The depth of printing, where v=0 prevents all printing
    void set_verbosity(unsigned int v);
    //! \brief If true, indents each line with a number of spaces equal to the level
    void set_indents_based_on_level(bool b);
    //! \brief If false, the level is shown on each first line of a print; if true, only when a print has a different level
    void set_prints_level_on_change_only(bool b);
    //! \brief If true, each entrance in a defined scope is logged
    void set_prints_scope_entrance(bool b);
    //! \brief If true, each exit from a defined scope is logged
    void set_prints_scope_exit(bool b);
    //! \brief If true, text that would be printed on multiple lines is correctly preambled on each line with level and indentation
    void set_handles_multiline_output(bool b);
    //! \brief If true, discards from printing all newlines and their immediately following whitespaces
    void set_discards_newlines_and_indentation(bool b);
    //! \brief Decides if and where to append the thread name to the log level (if the latter is shown)
    //! \details The policy is implicitly NEVER if the scheduler is immediate, or only one thread is registered (logging thread excluded)
    void set_thread_name_printing_policy(ThreadNamePrintingPolicy p);

    //! \brief Configuration getters

    unsigned int verbosity() const;
    /** @brief Test whether indentation follows the log level. */
    bool indents_based_on_level() const;
    /** @brief Test whether level labels are printed only when the level changes. */
    bool prints_level_on_change_only() const;
    /** @brief Test whether entering a scope produces a log message. */
    bool prints_scope_entrance() const;
    /** @brief Test whether leaving a scope produces a log message. */
    bool prints_scope_exit() const;
    /** @brief Test whether each output line receives a log preamble. */
    bool handles_multiline_output() const;
    /** @brief Test whether line breaks and following indentation are discarded. */
    bool discards_newlines_and_indentation() const;
    /** @brief Return the placement policy for thread names. */
    ThreadNamePrintingPolicy thread_name_printing_policy() const;

    //! \brief Style theme for terminal output
    void set_theme(const TerminalTextTheme& theme);
    //! \brief Get the current theme used
    TerminalTextTheme theme() const;
    //! \brief Add a keyword to the default ones offered, forcing a given style
    //! \details Adding an existing keyword has no effect
    void add_custom_keyword(const std::string& text, const TerminalTextStyle& style);
    //! \brief Add a keyword to the default ones offered, using the keyword style for the current theme
    //! \details Adding an existing keyword has no effect. Changing the theme will not apply to this custom keyword.
    void add_custom_keyword(const std::string& text);
    //! \brief Get the map of keywords (a TerminalTextStyle equal to TT_STYLE_NONE implies no custom style forced)
    std::map<std::string, TerminalTextStyle> custom_keywords() const;

    /** @brief Write the value to the output stream. */
    friend OutputStream& operator<<(OutputStream& os, const LoggerConfiguration& configuration);

  private:
    mutable std::mutex _mutex;
    unsigned int _verbosity;
    bool _indents_based_on_level;
    bool _prints_level_on_change_only;
    bool _prints_scope_entrance;
    bool _prints_scope_exit;
    bool _handles_multiline_output;
    bool _discards_newlines_and_indentation;
    ThreadNamePrintingPolicy _thread_name_printing_policy;

    TerminalTextTheme _theme;
    std::map<std::string, TerminalTextStyle> _custom_keywords;
};

class LoggerSchedulerInterface;

//! \brief A static class for log output handling.
//! Configuration and final printing is done here, while scheduling is
//! done in a separate class.
class Logger {
  private:
    friend class ImmediateLoggerScheduler;
    friend class BlockingLoggerScheduler;
    friend class NonblockingLoggerScheduler;

    /** @brief Construct a \c Logger with the specified characteristics. */
    Logger();

  public:
    /** @brief Shut down the logger and release the active scheduler. */
    ~Logger();
    /** @brief Disallow this construction to preserve the ownership contract. */
    Logger(const Logger&) = delete;
    /** @brief Disallow assignment to preserve the ownership contract. */
    void operator=(const Logger&) = delete;

    /** @brief Return the process-wide logger instance. */
    static Logger& instance();

    //! \brief Attach a thread registry, necessary to modify the scheduler and register/unregister
    void attach_thread_registry(ThreadRegistryInterface* registry);
    //! \brief Check whether there is already a thread registry attached
    bool has_thread_registry_attached() const;

    /** @brief Select synchronous output in the calling thread. */
    void use_immediate_scheduler();
    /** @brief Select the blocking concurrent log scheduler. */
    void use_blocking_scheduler();
    /** @brief Select the nonblocking concurrent log scheduler. */
    void use_nonblocking_scheduler();

    /** @brief Redirect log output to the named file. */
    void redirect_to_file(const char* filename);
    /** @brief Restore log output to the console. */
    void redirect_to_console();

    /** @brief Register the thread under the supplied display name. */
    void register_thread(std::thread::id id, std::string name);
    /** @brief Remove the thread from the logging registry. */
    void unregister_thread(std::thread::id id);
    //! \brief Registers this same thread with a specific \a name and \a level
    //! \details Required when the registering thread is the same as the one to register,
    //! since the level cannot be obtained
    void register_self_thread(std::string name, unsigned int level);

    /** @brief Submit a line at the current log level plus the supplied increase. */
    void println(unsigned int level_increase, std::string text);
    /** @brief Hold text at the bottom of the output for the named scope. */
    void hold(std::string scope, std::string text);
    /** @brief Release the held text associated with the named scope. */
    void release(std::string scope);

    /** @brief Increase the current thread log level. */
    void increase_level(unsigned int i);
    /** @brief Decrease the current thread log level. */
    void decrease_level(unsigned int i);
    /** @brief Increase the level by the mute offset. */
    void mute_increase_level();
    /** @brief Remove one mute-level offset. */
    void mute_decrease_level();

    /** @brief Test whether output at the supplied relative level is suppressed. */
    bool is_muted_at(unsigned int i) const;

    /** @brief Return the current thread log level. */
    unsigned int current_level() const;
    /** @brief Return the current thread level if registered with the active scheduler.
     *  Unregistered callers return std::nullopt instead of accessing missing thread data.
     */
    std::optional<unsigned int> try_current_level() const;
    /** @brief Return the current thread display name. */
    std::string current_thread_name() const;
    /** @brief Return the thread name used by the last printed line. */
    std::string cached_last_printed_thread_name() const;

    /** @brief Return the terminal width in columns. */
    unsigned int get_window_columns() const;

    /** @brief Return the logger configuration. */
    LoggerConfiguration& configuration();

  private:
    /** @brief Apply the configured terminal styles to the text. */
    std::string _apply_theme(const std::string& text) const;
    /** @brief Apply the configured keyword styles to the text. */
    std::string _apply_theme_for_keywords(const std::string& text) const;
    /** @brief Print the level and thread preamble for the first output line. */
    void _print_preamble_for_firstline(unsigned int level, std::string thread_name);
    /** @brief Print the preamble for continuation lines. */
    void _print_preamble_for_extralines(unsigned int level, SizeType thread_name_size);
    /** @brief Remove line breaks and their following indentation. */
    std::string _discard_newlines_and_indentation(const std::string& text);
    /** @brief Erase previously held terminal columns using spaces. */
    void _cover_held_columns_with_whitespaces(unsigned int printed_columns);
    /** @brief Redraw the currently held line. */
    void _print_held_line();
    /** @brief Submit a line at the current log level plus the supplied increase. */
    void _println(const LogRawMessage& msg);
    /** @brief Hold text at the bottom of the output for the named scope. */
    void _hold(const LogRawMessage& msg);
    /** @brief Release the held text associated with the named scope. */
    void _release(const LogRawMessage& msg);
    /** @brief Test whether a held-message scope is active. */
    bool _is_holding() const;
    /** @brief Test whether the active scheduler and registry allow thread-name output. */
    bool _can_print_thread_name() const;

  private:
    static const unsigned int _MUTE_LEVEL_OFFSET;
    static const std::string _MAIN_THREAD_NAME;
    std::ofstream _redirect_file;
    std::basic_streambuf<char>* _default_streambuf;
    mutable std::mutex _output_mutex;
    std::vector<LogRawMessage> _current_held_stack;
    unsigned int _cached_num_held_columns;
    unsigned int _cached_last_printed_level;
    std::string _cached_last_printed_thread_name;
    mutable std::shared_mutex _scheduler_mutex;
    std::shared_ptr<LoggerSchedulerInterface> _scheduler;
    ThreadRegistryInterface* _thread_registry;
    LoggerConfiguration _configuration;
};

} // namespace Ariadne

#endif // ARIADNE_LOGGING_LOGGING_HPP
