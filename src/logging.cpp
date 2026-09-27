/***************************************************************************
 *            logging.cpp
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

#include <iostream>
#include <cassert>
#include <thread>
#include <future>
#include <mutex>
#include <atomic>
#include <functional>

#ifndef _WIN32
#include <sys/ioctl.h>
#include <unistd.h>
#endif

#include "logging/logging.hpp"

namespace Ariadne {

class MessageConsumptionThread {
  public:
    MessageConsumptionThread(std::function<void()> task) : _started_future(_started_promise.get_future()) {
        _thread = std::thread([this, task]() { _started_promise.set_value(); task(); });
        _started_future.get();
    }
    ~MessageConsumptionThread() {  _thread.join(); }
  private:
    std::thread _thread;
    std::promise<void> _started_promise;
    std::future<void> _started_future;
};

TerminalTextStyle::TerminalTextStyle(uint8_t fontcolor_, uint8_t bgcolor_, bool bold_, bool underline_) :
    fontcolor(fontcolor_), bgcolor(bgcolor_), bold(bold_), underline(underline_)
{ }

std::string TerminalTextStyle::operator()() const {
    std::ostringstream ss;
    if (((int)fontcolor) > 0) ss << "\u001b[38;5;" << (int)fontcolor << "m";
    if (((int)bgcolor) > 0) ss << "\u001b[48;5;" << (int)bgcolor << "m";
    if (bold) ss << "\u001b[1m";
    if (underline) ss << "\u001b[4m";
    return ss.str();
}

bool TerminalTextStyle::is_styled() const {
    return ((int)fontcolor) > 0 or ((int)bgcolor) > 0 or bold or underline;
}

TerminalTextTheme::TerminalTextTheme(TerminalTextStyle level_number_, TerminalTextStyle level_shown_separator_, TerminalTextStyle level_hidden_separator_,
                                     TerminalTextStyle multiline_separator_, TerminalTextStyle assignment_comparison_, TerminalTextStyle miscellaneous_operator_,
                                     TerminalTextStyle round_parentheses_, TerminalTextStyle square_parentheses_, TerminalTextStyle curly_parentheses_,
                                     TerminalTextStyle colon_, TerminalTextStyle comma_, TerminalTextStyle number_, TerminalTextStyle at_, TerminalTextStyle keyword_) :
        level_number(level_number_), level_shown_separator(level_shown_separator_), level_hidden_separator(level_hidden_separator_),
        multiline_separator(multiline_separator_), assignment_comparison(assignment_comparison_), miscellaneous_operator(miscellaneous_operator_),
        round_parentheses(round_parentheses_), square_parentheses(square_parentheses_), curly_parentheses(curly_parentheses_),
        colon(colon_), comma(comma_), number(number_), at(at_), keyword(keyword_) { }

TerminalTextTheme::TerminalTextTheme() :
        level_number(TT_STYLE_NONE), level_shown_separator(TT_STYLE_NONE), level_hidden_separator(TT_STYLE_NONE),
        multiline_separator(TT_STYLE_NONE), assignment_comparison(TT_STYLE_NONE), miscellaneous_operator(TT_STYLE_NONE),
        round_parentheses(TT_STYLE_NONE), square_parentheses(TT_STYLE_NONE), curly_parentheses(TT_STYLE_NONE),
        colon(TT_STYLE_NONE), comma(TT_STYLE_NONE), number(TT_STYLE_NONE), at(TT_STYLE_NONE), keyword(TT_STYLE_NONE) { }

bool TerminalTextTheme::has_style() const {
    return (level_number.is_styled() or level_shown_separator.is_styled() or level_hidden_separator.is_styled() or
            multiline_separator.is_styled() or assignment_comparison.is_styled() or miscellaneous_operator.is_styled() or
            round_parentheses.is_styled() or square_parentheses.is_styled() or curly_parentheses.is_styled() or
            colon.is_styled() or comma.is_styled() or number.is_styled() or at.is_styled() or keyword.is_styled());
}

OutputStream& operator<<(OutputStream& os, TerminalTextTheme const& theme) {
    // This should not be used by LOGGING_PRINTLN, otherwise it will be parsed further, breaking level separator styling
    os << "TerminalTextTheme(\n  level_number= " << theme.level_number() << "1 2 3 4 5 6 7 8 9" << TerminalTextStyle::RESET;
    os << ",\n  level_shown_separator= " << theme.level_shown_separator() << "|" << TerminalTextStyle::RESET;
    os << ",\n  level_hidden_separator= " << theme.level_hidden_separator() << "|" << TerminalTextStyle::RESET;
    os << ",\n  multiline_separator= " << theme.multiline_separator() << "·" << TerminalTextStyle::RESET;
    os << ",\n  assignment_comparison= " << theme.assignment_comparison() << "= > < !" << TerminalTextStyle::RESET;
    os << ",\n  miscellaneous_operator= " << theme.miscellaneous_operator() << "+ - * / \\ ^ | & %" << TerminalTextStyle::RESET;
    os << ",\n  round_parentheses= " << theme.round_parentheses() << "( )" << TerminalTextStyle::RESET;
    os << ",\n  square_parentheses= " << theme.square_parentheses() << "[ ]" << TerminalTextStyle::RESET;
    os << ",\n  curly_parentheses= " << theme.curly_parentheses() << "{ }" << TerminalTextStyle::RESET;
    os << ",\n  comma= " << theme.comma() << ":" << TerminalTextStyle::RESET;
    os << ",\n  number= " << theme.number() << "1.2" << TerminalTextStyle::RESET;
    os << ",\n  at= " << theme.at() << ":" << TerminalTextStyle::RESET;
    os << ",\n  keyword= " << theme.keyword() << "virtual const true false inf" << TerminalTextStyle::RESET;
    os << "\n)";
    return os;
}

//! \brief Thread-based enqueued log data
class LoggerData {
    friend class NonblockingLoggerScheduler;
protected:
    LoggerData(unsigned int current_level, std::string const& thread_name);

    void enqueue_println(unsigned int level_increase, std::string text);
    void enqueue_hold(std::string scope, std::string text);
    void enqueue_release(std::string scope);

    LogThinRawMessage dequeue();

    void increase_level(unsigned int i);
    void decrease_level(unsigned int i);

    //! \brief Set the object to be removed as soon as empty because it's detached from its thread
    void kill();
    //! \brief Notifies if the related thread has been joined and the object can be safely removed as soon as empty
    bool is_dead() const;

    unsigned int current_level() const;
    std::string thread_name() const;

    SizeType queue_size() const;
private:
    unsigned int _current_level;
    std::string _thread_name;
    std::queue<LogThinRawMessage> _raw_messages;
    std::mutex _queue_mutex;
    bool _is_dead;
};

LogScopeManager::LogScopeManager(std::string scope, unsigned int level_increase)
    : _scope(scope), _level_increase(level_increase)
{
    Logger::instance().increase_level(_level_increase);
    if ((!Logger::instance().is_muted_at(0)) and Logger::instance().configuration().prints_scope_entrance()) {
        Logger::instance().println(0,"Enters '"+this->scope()+"'");
    }
}

std::string LogScopeManager::scope() const {
    return _scope;
}

LogScopeManager::~LogScopeManager() {
    if ((!Logger::instance().is_muted_at(0)) and Logger::instance().configuration().prints_scope_exit()) {
        Logger::instance().println(0,"Exits '"+this->scope()+"'");
    }
    Logger::instance().decrease_level(_level_increase);
    Logger::instance().release(this->scope());
}

LogThinRawMessage::LogThinRawMessage(std::string scope_, unsigned int level_, std::string text_) :
    scope(scope_), level(level_), text(text_)
{ }

RawMessageKind LogThinRawMessage::kind() const {
    if (scope.empty()) return RawMessageKind::PRINTLN;
    else if (!text.empty()) return RawMessageKind::HOLD;
    else return RawMessageKind::RELEASE;
}

LoggerData::LoggerData(unsigned int current_level, std::string const& thread_name)
    : _current_level(current_level), _thread_name(thread_name), _is_dead(false)
{ }

unsigned int LoggerData::current_level() const {
    return _current_level;
}

std::string LoggerData::thread_name() const {
    return _thread_name;
}

void LoggerData::enqueue_println(unsigned int level_increase, std::string text) {
    const std::lock_guard<std::mutex> lock(_queue_mutex);
    _raw_messages.push(LogThinRawMessage(std::string(), _current_level + level_increase, text));
}

void LoggerData::enqueue_hold(std::string scope, std::string text) {
    const std::lock_guard<std::mutex> lock(_queue_mutex);
    _raw_messages.push(LogThinRawMessage(scope, _current_level, text));
}

void LoggerData::enqueue_release(std::string scope) {
    const std::lock_guard<std::mutex> lock(_queue_mutex);
    _raw_messages.push(LogThinRawMessage(scope, _current_level, std::string()));
}

LogThinRawMessage LoggerData::dequeue() {
    const std::lock_guard<std::mutex> lock(_queue_mutex);
    LogThinRawMessage result = _raw_messages.front();
    _raw_messages.pop();
    return result;
}

void LoggerData::kill() {
    _is_dead = true;
}

bool LoggerData::is_dead() const {
    return _is_dead;
}

void LoggerData::increase_level(unsigned int i) {
    _current_level += i;
}

void LoggerData::decrease_level(unsigned int i) {
    _current_level -= i;
}

SizeType LoggerData::queue_size() const {
    return _raw_messages.size();
}

class LoggerSchedulerInterface {
  public:
    virtual void println(unsigned int level_increase, std::string text) = 0;
    virtual void hold(std::string scope, std::string text) = 0;
    virtual void release(std::string scope) = 0;
    virtual unsigned int current_level() const = 0;
    virtual std::string current_thread_name() const = 0;
    virtual SizeType largest_thread_name_size() const = 0;
    virtual void increase_level(unsigned int i) = 0;
    virtual void decrease_level(unsigned int i) = 0;
    virtual void terminate() = 0;
    virtual ~LoggerSchedulerInterface() = default;
};

//! \brief A Logger scheduler that prints immediately. Not designed for concurrency, since
//! the current level should be different for each thread that prints. Also the outputs can overlap arbitrarily.
class ImmediateLoggerScheduler : public LoggerSchedulerInterface {
  public:
    ImmediateLoggerScheduler();
    void println(unsigned int level_increase, std::string text) override;
    void hold(std::string scope, std::string text) override;
    void release(std::string scope) override;
    unsigned int current_level() const override;
    std::string current_thread_name() const override;
    SizeType largest_thread_name_size() const override;
    void increase_level(unsigned int i) override;
    void decrease_level(unsigned int i) override;
    void terminate() override;
  private:
    void _dequeue_pending();
  private:
    unsigned int _current_level;
};

//! \brief A Logger scheduler that enqueues messages and prints them sequentially.
//! The order of printing respects the order of submission, thus blocking other submitters.
class BlockingLoggerScheduler : public LoggerSchedulerInterface {
  public:
    BlockingLoggerScheduler();
    void println(unsigned int level_increase, std::string text) override;
    void hold(std::string scope, std::string text) override;
    void release(std::string scope) override;
    unsigned int current_level() const override;
    std::string current_thread_name() const override;
    SizeType largest_thread_name_size() const override;
    void increase_level(unsigned int i) override;
    void decrease_level(unsigned int i) override;
    void create_data_instance(std::thread::id id, std::string name);
    void create_data_instance(std::thread::id id, std::string name, unsigned int level);
    void kill_data_instance(std::thread::id id);
    void terminate() override;
  private:
    std::map<std::thread::id,std::pair<unsigned int,std::string>> _data;
    mutable std::mutex _data_mutex;
};

//! \brief A Logger scheduler that enqueues messages and prints them in a dedicated thread.
//! The order of printing is respected only within a given thread.
class NonblockingLoggerScheduler : public LoggerSchedulerInterface {
  public:
    NonblockingLoggerScheduler();
    void println(unsigned int level_increase, std::string text) override;
    void hold(std::string scope, std::string text) override;
    void release(std::string scope) override;
    unsigned int current_level() const override;
    std::string current_thread_name() const override;
    SizeType largest_thread_name_size() const override;
    void increase_level(unsigned int i) override;
    void decrease_level(unsigned int i) override;
    void create_data_instance(std::thread::id id, std::string name);
    void create_data_instance(std::thread::id id, std::string name, unsigned int level);
    void kill_data_instance(std::thread::id id);
    void terminate() override;
    ~NonblockingLoggerScheduler() override;
  private:
    //! \brief Extracts one message from the largest queue. Requires _data_mutex to be held.
    LogRawMessage _dequeue_unlocked();
    void _consume_msgs();
    bool _is_queue_empty_unlocked() const;
    bool _are_alive_threads_registered_unlocked() const;
 private:
    std::condition_variable _message_availability_condition;
    mutable std::mutex _data_mutex;

    bool _terminate;
    bool _no_alive_thread_registered;
    std::promise<void> _termination_promise;
    std::future<void> _termination_future;
    SharedPointer<MessageConsumptionThread> _dequeueing_thread;
    std::map<std::thread::id,SharedPointer<LoggerData>> _data;
};

ImmediateLoggerScheduler::ImmediateLoggerScheduler() : _current_level(1) { }

unsigned int ImmediateLoggerScheduler::current_level() const {
    return _current_level;
}

std::string ImmediateLoggerScheduler::current_thread_name() const {
    std::stringstream ss;
    ss << std::this_thread::get_id();
    return ss.str();
}

SizeType ImmediateLoggerScheduler::largest_thread_name_size() const {
    return current_thread_name().size();
}

void ImmediateLoggerScheduler::increase_level(unsigned int i) {
    _current_level+=i;
}

void ImmediateLoggerScheduler::decrease_level(unsigned int i) {
    _current_level-=i;
}

void ImmediateLoggerScheduler::println(unsigned int level_increase, std::string text) {
    Logger::instance()._println(LogRawMessage(std::string(), _current_level + level_increase, text));
}

void ImmediateLoggerScheduler::hold(std::string scope, std::string text) {
    Logger::instance()._hold(LogRawMessage(scope, _current_level, text));
}

void ImmediateLoggerScheduler::release(std::string scope) {
    Logger::instance()._release(LogRawMessage(scope, _current_level, std::string()));
}

void ImmediateLoggerScheduler::terminate() { }

BlockingLoggerScheduler::BlockingLoggerScheduler() {
    _data.insert({std::this_thread::get_id(),std::make_pair(1,Logger::_MAIN_THREAD_NAME)});
}

void BlockingLoggerScheduler::create_data_instance(std::thread::id id, std::string name) {
    create_data_instance(id,name,current_level());
}

void BlockingLoggerScheduler::create_data_instance(std::thread::id id, std::string name, unsigned int level) {
    std::lock_guard<std::mutex> lock(_data_mutex);
    // Won't replace if it already exists
    _data.insert({id,make_pair(level,name)});
}

void BlockingLoggerScheduler::kill_data_instance(std::thread::id id) {
    std::unique_lock<std::mutex> lock(_data_mutex);
    auto entry = _data.find(id);
    if (entry != _data.end()) _data.erase(entry);
}

unsigned int BlockingLoggerScheduler::current_level() const {
    std::lock_guard<std::mutex> lock(_data_mutex);
    return _data.find(std::this_thread::get_id())->second.first;
}

std::string BlockingLoggerScheduler::current_thread_name() const {
    std::lock_guard<std::mutex> lock(_data_mutex);
    return _data.find(std::this_thread::get_id())->second.second;
}

SizeType BlockingLoggerScheduler::largest_thread_name_size() const {
    std::lock_guard<std::mutex> lock(_data_mutex);
    SizeType result = 0;
    for (auto entry : _data) result = std::max(result,entry.second.second.size());
    return result;
}

void BlockingLoggerScheduler::increase_level(unsigned int i) {
    std::lock_guard<std::mutex> lock(_data_mutex);
    _data.find(std::this_thread::get_id())->second.first += i;
}

void BlockingLoggerScheduler::decrease_level(unsigned int i) {
    std::lock_guard<std::mutex> lock(_data_mutex);
    _data.find(std::this_thread::get_id())->second.first -= i;
}

void BlockingLoggerScheduler::println(unsigned int level_increase, std::string text) {
    LogRawMessage msg(std::string(), 0, std::string());
    {
        std::lock_guard<std::mutex> lock(_data_mutex);
        auto const& data = _data.find(std::this_thread::get_id())->second;
        msg = LogRawMessage(data.second, std::string(), data.first + level_increase, text);
    }
    Logger::instance()._println(msg);
}

void BlockingLoggerScheduler::hold(std::string scope, std::string text) {
    LogRawMessage msg(std::string(), 0, std::string());
    {
        std::lock_guard<std::mutex> lock(_data_mutex);
        auto const& data = _data.find(std::this_thread::get_id())->second;
        msg = LogRawMessage(data.second, scope, data.first, text);
    }
    Logger::instance()._hold(msg);
}

void BlockingLoggerScheduler::release(std::string scope) {
    LogRawMessage msg(std::string(), 0, std::string());
    {
        std::lock_guard<std::mutex> lock(_data_mutex);
        auto const& data = _data.find(std::this_thread::get_id())->second;
        msg = LogRawMessage(data.second, scope, data.first, std::string());
    }
    Logger::instance()._release(msg);
}

void BlockingLoggerScheduler::terminate() { }

NonblockingLoggerScheduler::NonblockingLoggerScheduler() : _terminate(false), _no_alive_thread_registered(true), _termination_future(_termination_promise.get_future()) {
    {
        std::lock_guard<std::mutex> lock(_data_mutex);
        _data.insert({std::this_thread::get_id(),SharedPointer<LoggerData>(new LoggerData(1,Logger::_MAIN_THREAD_NAME))});
    }
    _dequeueing_thread = std::make_shared<MessageConsumptionThread>([this] { _consume_msgs(); });
}

void NonblockingLoggerScheduler::terminate() {
    {
        std::lock_guard<std::mutex> lock(_data_mutex);
        _terminate = true;
    }
    _message_availability_condition.notify_one();
    _termination_future.get();
}

NonblockingLoggerScheduler::~NonblockingLoggerScheduler() { }

void NonblockingLoggerScheduler::create_data_instance(std::thread::id id, std::string name) {
    create_data_instance(id,name,current_level());
}

void NonblockingLoggerScheduler::create_data_instance(std::thread::id id, std::string name, unsigned int level) {
    {
        std::unique_lock<std::mutex> lock(_data_mutex);
        // A std::thread::id may be reused after a thread terminates. If the old
        // thread still has queued messages, wait until the consumer drains and
        // removes its dead LoggerData before registering the new thread.
        _message_availability_condition.wait(lock, [this,id] {
            return _data.find(id) == _data.end();
        });
        _data.insert({id,SharedPointer<LoggerData>(new LoggerData(level,name))});
        _no_alive_thread_registered = false;
    }
    _message_availability_condition.notify_one();
}

void NonblockingLoggerScheduler::kill_data_instance(std::thread::id id) {
    {
        std::lock_guard<std::mutex> lock(_data_mutex);
        auto entry = _data.find(id);
        if (entry != _data.end()) {
            entry->second->kill();
            if (entry->second->queue_size() == 0) _data.erase(entry);
        }

        _no_alive_thread_registered = not _are_alive_threads_registered_unlocked();
    }
    _message_availability_condition.notify_one();
}

bool NonblockingLoggerScheduler::_are_alive_threads_registered_unlocked() const {
    for (auto const& d : _data)
        if (d.second->thread_name() != Logger::_MAIN_THREAD_NAME and not d.second->is_dead()) return true;
    return false;
}

unsigned int NonblockingLoggerScheduler::current_level() const {
    std::lock_guard<std::mutex> lock(_data_mutex);
    return _data.find(std::this_thread::get_id())->second->current_level();
}

std::string NonblockingLoggerScheduler::current_thread_name() const {
    std::lock_guard<std::mutex> lock(_data_mutex);
    return _data.find(std::this_thread::get_id())->second->thread_name();
}

SizeType NonblockingLoggerScheduler::largest_thread_name_size() const {
    std::lock_guard<std::mutex> lock(_data_mutex);
    SizeType result = 0;
    for (auto const& entry : _data) result = std::max(result,entry.second->thread_name().size());
    return result;
}

void NonblockingLoggerScheduler::increase_level(unsigned int i) {
    std::lock_guard<std::mutex> lock(_data_mutex);
    _data.find(std::this_thread::get_id())->second->increase_level(i);
}

void NonblockingLoggerScheduler::decrease_level(unsigned int i) {
    std::lock_guard<std::mutex> lock(_data_mutex);
    _data.find(std::this_thread::get_id())->second->decrease_level(i);
}

void NonblockingLoggerScheduler::println(unsigned int level_increase, std::string text) {
    {
        std::lock_guard<std::mutex> lock(_data_mutex);
        _data.find(std::this_thread::get_id())->second->enqueue_println(level_increase,text);
    }
    _message_availability_condition.notify_one();
}

void NonblockingLoggerScheduler::hold(std::string scope, std::string text) {
    {
        std::lock_guard<std::mutex> lock(_data_mutex);
        _data.find(std::this_thread::get_id())->second->enqueue_hold(scope,text);
    }
    _message_availability_condition.notify_one();
}

void NonblockingLoggerScheduler::release(std::string scope) {
    {
        std::lock_guard<std::mutex> lock(_data_mutex);
        _data.find(std::this_thread::get_id())->second->enqueue_release(scope);
    }
    _message_availability_condition.notify_one();
}

bool NonblockingLoggerScheduler::_is_queue_empty_unlocked() const {
    for (auto const& entry : _data) {
        if (entry.second->queue_size() > 0) return false;
    }
    return true;
}

LogRawMessage NonblockingLoggerScheduler::_dequeue_unlocked() {
    auto largest_it = _data.end();
    SizeType largest_size = 0;
    for (auto it = _data.begin(); it != _data.end(); ++it) {
        SizeType size = it->second->queue_size();
        if (size > largest_size) {
            largest_it = it;
            largest_size = size;
        }
    }

    auto const thread_name = largest_it->second->thread_name();
    auto msg = largest_it->second->dequeue();
    if (largest_it->second->is_dead() and largest_it->second->queue_size() == 0) {
        _data.erase(largest_it);
        _message_availability_condition.notify_all();
    }
    return LogRawMessage(thread_name,msg);
}

void NonblockingLoggerScheduler::_consume_msgs() {
    while(true) {
        LogRawMessage msg(std::string(), 0, std::string());
        {
            std::unique_lock<std::mutex> lock(_data_mutex);
            _message_availability_condition.wait(lock, [this] {
                return _terminate or not _is_queue_empty_unlocked();
            });
            if (_terminate and _is_queue_empty_unlocked()) {
                _termination_promise.set_value();
                return;
            }
            msg = _dequeue_unlocked();
        }

        if (msg.kind() == RawMessageKind::PRINTLN) Logger::instance()._println(msg);
        else if (msg.kind() == RawMessageKind::HOLD) Logger::instance()._hold(msg);
        else Logger::instance()._release(msg);
    }
}

OutputStream& operator<<(OutputStream& os, const ThreadNamePrintingPolicy& p) {
    switch(p) {
        default : [[fallthrough]];
        case ThreadNamePrintingPolicy::NEVER : os << "NEVER"; break;
        case ThreadNamePrintingPolicy::BEFORE : os << "BEFORE"; break;
        case ThreadNamePrintingPolicy::AFTER : os << "AFTER"; break;
    }
    return os;
}

LoggerConfiguration::LoggerConfiguration() :
        _verbosity(0), _indents_based_on_level(true), _prints_level_on_change_only(true), _prints_scope_entrance(false),
        _prints_scope_exit(false), _handles_multiline_output(true), _discards_newlines_and_indentation(false),
        _thread_name_printing_policy(ThreadNamePrintingPolicy::NEVER),
        _theme(TT_THEME_NONE)
{ }

LoggerConfiguration& Logger::configuration() {
    return _configuration;
}

void LoggerConfiguration::set_verbosity(unsigned int v) {
    std::lock_guard<std::mutex> lock(_mutex);
    _verbosity = v;
}

void LoggerConfiguration::set_indents_based_on_level(bool b) {
    std::lock_guard<std::mutex> lock(_mutex);
    _indents_based_on_level = b;
}

void LoggerConfiguration::set_prints_level_on_change_only(bool b) {
    std::lock_guard<std::mutex> lock(_mutex);
    _prints_level_on_change_only = b;
}

void LoggerConfiguration::set_prints_scope_entrance(bool b) {
    std::lock_guard<std::mutex> lock(_mutex);
    _prints_scope_entrance = b;
}

void LoggerConfiguration::set_prints_scope_exit(bool b) {
    std::lock_guard<std::mutex> lock(_mutex);
    _prints_scope_exit = b;
}

void LoggerConfiguration::set_handles_multiline_output(bool b) {
    std::lock_guard<std::mutex> lock(_mutex);
    _handles_multiline_output = b;
}

void LoggerConfiguration::set_discards_newlines_and_indentation(bool b) {
    std::lock_guard<std::mutex> lock(_mutex);
    _discards_newlines_and_indentation = b;
}

void LoggerConfiguration::set_thread_name_printing_policy(ThreadNamePrintingPolicy p) {
    std::lock_guard<std::mutex> lock(_mutex);
    _thread_name_printing_policy = p;
}

void LoggerConfiguration::set_theme(TerminalTextTheme const& theme) {
    std::lock_guard<std::mutex> lock(_mutex);
    _theme = theme;
}

unsigned int LoggerConfiguration::verbosity() const {
    std::lock_guard<std::mutex> lock(_mutex);
    return _verbosity;
}

bool LoggerConfiguration::indents_based_on_level() const {
    std::lock_guard<std::mutex> lock(_mutex);
    return _indents_based_on_level;
}

bool LoggerConfiguration::prints_level_on_change_only() const {
    std::lock_guard<std::mutex> lock(_mutex);
    return _prints_level_on_change_only;
}

bool LoggerConfiguration::prints_scope_entrance() const {
    std::lock_guard<std::mutex> lock(_mutex);
    return _prints_scope_entrance;
}

bool LoggerConfiguration::prints_scope_exit() const {
    std::lock_guard<std::mutex> lock(_mutex);
    return _prints_scope_exit;
}

bool LoggerConfiguration::handles_multiline_output() const {
    std::lock_guard<std::mutex> lock(_mutex);
    return _handles_multiline_output;
}

bool LoggerConfiguration::discards_newlines_and_indentation() const {
    std::lock_guard<std::mutex> lock(_mutex);
    return _discards_newlines_and_indentation;
}

ThreadNamePrintingPolicy LoggerConfiguration::thread_name_printing_policy() const {
    std::lock_guard<std::mutex> lock(_mutex);
    return _thread_name_printing_policy;
}

TerminalTextTheme LoggerConfiguration::theme() const {
    std::lock_guard<std::mutex> lock(_mutex);
    return _theme;
}

void LoggerConfiguration::add_custom_keyword(std::string const& text, TerminalTextStyle const& style) {
    std::lock_guard<std::mutex> lock(_mutex);
    _custom_keywords.insert({text,style});
}

void LoggerConfiguration::add_custom_keyword(std::string const& text) {
    std::lock_guard<std::mutex> lock(_mutex);
    _custom_keywords.insert({text,_theme.keyword});
}

std::map<std::string,TerminalTextStyle> LoggerConfiguration::custom_keywords() const {
    std::lock_guard<std::mutex> lock(_mutex);
    return _custom_keywords;
}

OutputStream& operator<<(OutputStream& os, LoggerConfiguration const& c) {
    os << "LoggerConfiguration("
       << "\n  verbosity=" << c.verbosity()
       << ",\n  indents_based_on_level=" << c.indents_based_on_level()
       << ",\n  prints_level_on_change_only=" << c.prints_level_on_change_only()
       << ",\n  prints_scope_entrance=" << c.prints_scope_entrance()
       << ",\n  prints_scope_exit=" << c.prints_scope_exit()
       << ",\n  handles_multiline_output=" << c.handles_multiline_output()
       << ",\n  discards_newlines_and_indentation=" << c.discards_newlines_and_indentation()
       << ",\n  thread_name_printing_policy=" << c.thread_name_printing_policy()
       << ",\n  theme=(not shown)" // To show theme colors appropriately, print the theme object directly on standard output
       << "\n)";
    return os;
}

Logger::Logger() :
    _default_streambuf(std::clog.rdbuf()),
    _cached_num_held_columns(0), _cached_last_printed_level(0), _cached_last_printed_thread_name(std::string()),
    _scheduler(std::make_shared<NonblockingLoggerScheduler>()), _thread_registry(nullptr) { }

Logger& Logger::instance() {
#ifdef _WIN32
    // Keep the logger alive for the process lifetime. Destroying a Logger whose
    // nonblocking scheduler owns a worker thread during DLL teardown can
    // deadlock under the Windows loader lock.
    static Logger* instance = new Logger();
    return *instance;
#else
    static Logger instance;
    return instance;
#endif
}

const std::string Logger::_MAIN_THREAD_NAME = "main";
const unsigned int Logger::_MUTE_LEVEL_OFFSET = 1024;

Logger::~Logger() {
    std::unique_lock<std::shared_mutex> lock(_scheduler_mutex);
    _scheduler->terminate();
}

void Logger::attach_thread_registry(ThreadRegistryInterface* registry) {
    if (_thread_registry != nullptr) throw LoggerModifyThreadRegistryException();
    _thread_registry = registry;
}

bool Logger::has_thread_registry_attached() const {
    return _thread_registry != nullptr;
}

void Logger::use_immediate_scheduler() {
    std::unique_lock<std::shared_mutex> lock(_scheduler_mutex);
    if (not has_thread_registry_attached()) throw LoggerNoThreadRegistryException();
    if (_thread_registry->has_threads_registered()) throw LoggerSchedulerChangeWithRegisteredThreadsException();
    _scheduler->terminate();
    _scheduler.reset(new ImmediateLoggerScheduler());
}

void Logger::use_blocking_scheduler() {
    std::unique_lock<std::shared_mutex> lock(_scheduler_mutex);
    if (not has_thread_registry_attached()) throw LoggerNoThreadRegistryException();
    if (_thread_registry->has_threads_registered()) throw LoggerSchedulerChangeWithRegisteredThreadsException();
    _scheduler->terminate();
    _scheduler.reset(new BlockingLoggerScheduler());
}

void Logger::use_nonblocking_scheduler() {
    std::unique_lock<std::shared_mutex> lock(_scheduler_mutex);
    if (not has_thread_registry_attached()) throw LoggerNoThreadRegistryException();
    if (_thread_registry->has_threads_registered()) throw LoggerSchedulerChangeWithRegisteredThreadsException();
    _scheduler->terminate();
    _scheduler.reset(new NonblockingLoggerScheduler());
}

void Logger::redirect_to_console() {
    if(_redirect_file.is_open()) _redirect_file.close();
    std::clog.rdbuf(_default_streambuf);
}

void Logger::redirect_to_file(const char* filename) {
    if(_redirect_file.is_open()) _redirect_file.close();
    _redirect_file.open(filename);
    _default_streambuf = std::clog.rdbuf();
    std::clog.rdbuf( _redirect_file.rdbuf() );
}

void Logger::register_thread(std::thread::id id, std::string name) {
    std::shared_lock<std::shared_mutex> lock(_scheduler_mutex);
    if (not has_thread_registry_attached()) throw LoggerNoThreadRegistryException();
    auto nbls = dynamic_cast<NonblockingLoggerScheduler*>(_scheduler.get());
    auto bls = dynamic_cast<BlockingLoggerScheduler*>(_scheduler.get());
    if (nbls != nullptr) nbls->create_data_instance(id,name);
    else if (bls != nullptr) bls->create_data_instance(id,name);
}

void Logger::register_self_thread(std::string name, unsigned int level) {
    std::shared_lock<std::shared_mutex> lock(_scheduler_mutex);
    if (not has_thread_registry_attached()) throw LoggerNoThreadRegistryException();
    auto nbls = dynamic_cast<NonblockingLoggerScheduler*>(_scheduler.get());
    auto bls = dynamic_cast<BlockingLoggerScheduler*>(_scheduler.get());
    if (nbls != nullptr) nbls->create_data_instance(std::this_thread::get_id(),name,level);
    else if (bls != nullptr) bls->create_data_instance(std::this_thread::get_id(),name,level);
}

void Logger::unregister_thread(std::thread::id id) {
    std::shared_lock<std::shared_mutex> lock(_scheduler_mutex);
    if (not has_thread_registry_attached()) throw LoggerNoThreadRegistryException();
    auto nbls = dynamic_cast<NonblockingLoggerScheduler*>(_scheduler.get());
    auto bls = dynamic_cast<BlockingLoggerScheduler*>(_scheduler.get());
    if (nbls != nullptr) nbls->kill_data_instance(id);
    else if (bls != nullptr) bls->kill_data_instance(id);
}

void Logger::increase_level(unsigned int i) {
    std::shared_lock<std::shared_mutex> lock(_scheduler_mutex);
    _scheduler->increase_level(i);
}

void Logger::decrease_level(unsigned int i) {
    std::shared_lock<std::shared_mutex> lock(_scheduler_mutex);
    _scheduler->decrease_level(i);
}

void Logger::mute_increase_level() {
    std::shared_lock<std::shared_mutex> lock(_scheduler_mutex);
    _scheduler->increase_level(_MUTE_LEVEL_OFFSET);
}

void Logger::mute_decrease_level() {
    std::shared_lock<std::shared_mutex> lock(_scheduler_mutex);
    _scheduler->decrease_level(_MUTE_LEVEL_OFFSET);
}

bool Logger::is_muted_at(unsigned int i) const {
    return (_configuration.verbosity() < current_level()+i);
}

unsigned int Logger::current_level() const {
    std::shared_lock<std::shared_mutex> lock(_scheduler_mutex);
    return _scheduler->current_level();
}

std::string Logger::current_thread_name() const {
    std::shared_lock<std::shared_mutex> lock(_scheduler_mutex);
    return _scheduler->current_thread_name();
}

std::string Logger::cached_last_printed_thread_name() const {
    std::lock_guard<std::mutex> lock(_output_mutex);
    return _cached_last_printed_thread_name;
}

void Logger::println(unsigned int level_increase, std::string text) {
    std::shared_lock<std::shared_mutex> lock(_scheduler_mutex);
    _scheduler->println(level_increase, text);
}

void Logger::hold(std::string scope, std::string text) {
    std::shared_lock<std::shared_mutex> lock(_scheduler_mutex);
    _scheduler->hold(scope, text);
}

void Logger::release(std::string scope) {
    std::shared_lock<std::shared_mutex> lock(_scheduler_mutex);
    _scheduler->release(scope);
}

bool Logger::_is_holding() const {
    return !_current_held_stack.empty();
}

bool Logger::_can_print_thread_name() const {
    auto sch = dynamic_cast<ImmediateLoggerScheduler*>(_scheduler.get());
    // Only if we don't use an immediate scheduler and we have the right printing policy
    if (sch == nullptr and _configuration.thread_name_printing_policy() != ThreadNamePrintingPolicy::NEVER)
        return true;
    else return false;
}

unsigned int Logger::get_window_columns() const {
    const unsigned int DEFAULT_COLUMNS = 80;
    #ifndef _WIN32
        const unsigned int MAX_COLUMNS = 512;
        struct winsize ws;
        ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
        return ((ws.ws_col > 0 and ws.ws_col <= MAX_COLUMNS) ? ws.ws_col : DEFAULT_COLUMNS);
    #else
        return DEFAULT_COLUMNS;
    #endif
}

std::string Logger::_apply_theme(std::string const& text) const {
    auto theme = _configuration.theme();
    if (theme.has_style()) {
        std::ostringstream ss;
        for(auto it = text.begin(); it != text.end(); ++it) {
            const char& c = *it;
            switch (c) {
                case '=':
                case '>':
                case '<':
                case '!':
                    ss << theme.assignment_comparison() << c << TerminalTextStyle::RESET;
                    break;
                case '(':
                case ')':
                    ss << theme.round_parentheses() << c << TerminalTextStyle::RESET;
                    break;
                case '[':
                case ']':
                    ss << theme.square_parentheses() << c << TerminalTextStyle::RESET;
                    break;
                case '{':
                case '}':
                    ss << theme.curly_parentheses() << c << TerminalTextStyle::RESET;
                    break;
                case ':':
                    ss << theme.colon() << c << TerminalTextStyle::RESET;
                    break;
                case ',':
                    ss << theme.comma() << c << TerminalTextStyle::RESET;
                    break;
                case '@':
                    ss << theme.at() << c << TerminalTextStyle::RESET;
                    break;
                case '.':
                    if (it != text.begin() and isdigit(*(it-1)))
                        ss << theme.number() << c << TerminalTextStyle::RESET;
                    else
                        ss << c;
                    break;
                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':
                {
                    // Exclude strings that end with a number (supported up to 2 digits) to account for numbered variables
                    // For simplicity, this does not work across multiple lines
                    bool styled = true;
                    if (it != text.begin()) {
                        if (isalpha(*(it - 1)))
                            styled = false;
                        else if (isdigit(*(it - 1)) and isalpha(*(it - 2)))
                            styled = false;
                    }
                    if (styled) ss << theme.number() << c << TerminalTextStyle::RESET;
                    else ss << c;
                    break;
                }
                case '+':
                case '-':
                case '*':
                case '/':
                case '\\':
                case '^':
                case '|':
                case '&':
                case '%':
                    ss << theme.miscellaneous_operator() << c << TerminalTextStyle::RESET;
                    break;
                default:
                    ss << c;
            }
        }
        return _apply_theme_for_keywords(ss.str());
    } else return text;
}

bool isalphanumeric_withstylecodes(std::string const& text, size_t pos) {
    auto c = text.at(pos);
    if (not isalpha(c) and not isdigit(c)) return false;
    if (c != 'm' or pos <= 3) return true;

    // A reset code ends with ESC[0m. In that case adjacency is determined by
    // the character immediately preceding the four-byte reset sequence.
    if (text.compare(pos-3,3,"\u001b[0") == 0) {
        auto preceding = text.at(pos-4);
        return std::isalnum(static_cast<unsigned char>(preceding)) != 0;
    }
    return true;
}

std::string Logger::_apply_theme_for_keywords(std::string const& text) const {
    std::string result = text;
    std::map<std::string,TerminalTextStyle> keyword_styles = {{"virtual",_configuration.theme().keyword},
                                                              {"const",_configuration.theme().keyword},
                                                              {"true",_configuration.theme().keyword},
                                                              {"false",_configuration.theme().keyword},
                                                              {"inf",_configuration.theme().keyword}};
    auto custom_keywords = _configuration.custom_keywords();
    keyword_styles.insert(custom_keywords.begin(),custom_keywords.end());

    for (auto kws : keyword_styles) {
        std::ostringstream current_result;
        size_t kw_length = kws.first.length();
        size_t kw_pos = std::string::npos;
        size_t scan_pos = 0;
        while ((kw_pos  = result.find(kws.first,scan_pos)) != std::string::npos) {
            // Apply the theme only if the keyword is not adjacent to an alphanumerical character
            if ((kw_pos > 0 and isalphanumeric_withstylecodes(result,kw_pos-1)) or
                (kw_pos+kw_length < result.length() and (isalpha(result.at(kw_pos+kw_length)) or isdigit(result.at(kw_pos+kw_length)))))
                current_result << result.substr(scan_pos,kw_pos-scan_pos+kw_length);
            else
                current_result << result.substr(scan_pos,kw_pos-scan_pos) << kws.second() << result.substr(kw_pos,kw_length) << TerminalTextStyle::RESET;

            scan_pos = kw_pos+kw_length;
        }
        if (scan_pos != 0) {
            if (scan_pos < result.length()) current_result << result.substr(scan_pos);
            result = current_result.str();
        }
    }
    return result;
}

void Logger::_print_preamble_for_firstline(unsigned int level, std::string thread_name) {
    auto theme = _configuration.theme();
    bool can_print_thread_name = _can_print_thread_name();
    bool thread_name_changed = (_cached_last_printed_thread_name != thread_name);
    bool level_changed = (_cached_last_printed_level != level);
    bool always_print_level = not(_configuration.prints_level_on_change_only());
    auto largest_thread_name_size = std::max(_scheduler->largest_thread_name_size(),thread_name.size());
    std::string thread_name_prefix = std::string(largest_thread_name_size-thread_name.size(),' ');

    if (can_print_thread_name and _configuration.thread_name_printing_policy() == ThreadNamePrintingPolicy::BEFORE) {
        if (thread_name_changed) {
            if (theme.at.is_styled()) std::clog << thread_name_prefix << thread_name << theme.at() << "@" << TerminalTextStyle::RESET;
            else std::clog << thread_name_prefix << thread_name << "@";
        } else std::clog << std::string(largest_thread_name_size+1, ' ');
    }

    if ((can_print_thread_name and thread_name_changed) or always_print_level or level_changed) {
        if (theme.level_number.is_styled()) std::clog << theme.level_number() << level << TerminalTextStyle::RESET;
        else std::clog << level;
    } else std::clog << (level>9 ? "  " : " ");

    if (can_print_thread_name and _configuration.thread_name_printing_policy() == ThreadNamePrintingPolicy::AFTER) {
        if (thread_name_changed) {
            if (theme.at.is_styled()) std::clog << theme.at() << "@" << TerminalTextStyle::RESET << thread_name;
            else std::clog << "@" << thread_name;
        } else std::clog << std::string(largest_thread_name_size+1, ' ');
    }

    if (not level_changed and _configuration.prints_level_on_change_only() and theme.level_hidden_separator.is_styled()) {
        std::clog << theme.level_hidden_separator() << "|" << TerminalTextStyle::RESET;
    } else if ((level_changed and theme.level_shown_separator.is_styled()) or not _configuration.prints_level_on_change_only()) {
        std::clog << theme.level_shown_separator() << "|" << TerminalTextStyle::RESET;
    } else {
        std::clog << "|";
    }
    if (_configuration.indents_based_on_level()) std::clog << std::string(level, ' ');
}

void Logger::_print_preamble_for_extralines(unsigned int level, SizeType thread_name_size) {
    auto theme = _configuration.theme();
    std::clog << (level>9 ? "  " : " ");
    if (_can_print_thread_name()) {
        auto largest_thread_name_size = std::max(_scheduler->largest_thread_name_size(),thread_name_size);
        std::clog << std::string(largest_thread_name_size + 1, ' ');
    }
    if (theme.multiline_separator.is_styled()) std::clog << theme.multiline_separator() << "·" << TerminalTextStyle::RESET;
    else std::clog << "·";

    if (_configuration.indents_based_on_level()) std::clog << std::string(level, ' ');
}

std::string Logger::_discard_newlines_and_indentation(std::string const& text) {
    std::ostringstream result;
    size_t text_ptr = 0;
    while(true) {
        std::size_t newline_pos = text.find('\n',text_ptr);
        if (newline_pos != std::string::npos) {
            size_t i=0;
            while(true) {
                if (text.at(newline_pos+1+i) == ' ') ++i;
                else break;
            }
            result << text.substr(text_ptr,newline_pos-text_ptr);
            text_ptr = newline_pos+1+i;
        } else {
            result << text.substr(text_ptr);
            break;
        }
    }
    return result.str();
}

void Logger::_print_held_line() {
    auto theme = _configuration.theme();
    const unsigned int max_columns = get_window_columns();
    unsigned int held_columns = 0;

    std::clog << '\r';
    for (auto msg : _current_held_stack) {
        held_columns = held_columns+(msg.level>9 ? 2 : 1)+3+static_cast<unsigned int>(msg.text.size());
        if (held_columns>max_columns+1) {
            std::string original = theme.level_number() + std::to_string(msg.level) + TerminalTextStyle::RESET +
                                   theme.level_shown_separator() + "|" + TerminalTextStyle::RESET + " " + _apply_theme(msg.text) + " ";
            std::clog << original.substr(0,original.size()-(held_columns-max_columns+2)) << "..";
            held_columns=max_columns;
            break;
        } else if(held_columns==max_columns || held_columns==max_columns+1) {
            std::clog << theme.level_number() << msg.level << TerminalTextStyle::RESET <<
                         theme.level_shown_separator() << "|" << TerminalTextStyle::RESET << " " << _apply_theme(msg.text);
            held_columns=max_columns;
            break;
        } else {
            std::clog << theme.level_number() << msg.level << TerminalTextStyle::RESET <<
                         theme.level_shown_separator() << "|" << TerminalTextStyle::RESET << " " << _apply_theme(msg.text) << " ";
        }
    }
    std::clog << std::flush;
    _cached_num_held_columns=held_columns;
    // Sleep for a time exponential in the last printed level (used as an average of the levels printed)
    // In this way, holding is printed more cleanly against the OS buffering, which overrules flushing
    std::this_thread::sleep_for(std::chrono::microseconds(10<<_cached_last_printed_level));
}

void Logger::_cover_held_columns_with_whitespaces(unsigned int printed_columns) {
    if (_is_holding()) {
        if (_cached_num_held_columns > printed_columns)
            std::clog << std::string(_cached_num_held_columns - printed_columns, ' ');
    }
}

void Logger::_println(LogRawMessage const& msg) {
    std::lock_guard<std::mutex> lock(_output_mutex);
    const auto largest_thread_name_size = std::max(_scheduler->largest_thread_name_size(),msg.identifier.size());
    const unsigned int preamble_columns = (msg.level>9 ? 3:2)+(_can_print_thread_name() ? static_cast<unsigned int>(largest_thread_name_size+1) : 0)+msg.level;
    // If holding, we must write over the held line first
    if (_is_holding()) std::clog << '\r';

    _print_preamble_for_firstline(msg.level,msg.identifier);
    std::string text = msg.text;
    if (configuration().discards_newlines_and_indentation()) text = _discard_newlines_and_indentation(text);
    if (configuration().handles_multiline_output() and msg.text.size() > 0) {
        const unsigned int max_columns = get_window_columns();
        size_t text_ptr = 0;
        const size_t text_size = text.size();
        while(true) {
            if (text_size-text_ptr + preamble_columns > max_columns) { // (remaining) Text too long for a single terminal line
                std::string to_print = text.substr(text_ptr,max_columns-preamble_columns);
                std::size_t newline_pos = to_print.find('\n');
                if (newline_pos != std::string::npos) { // A newline is found before reaching the end of the terminal line
                    std::clog << _apply_theme(to_print.substr(0,newline_pos));
                    _cover_held_columns_with_whitespaces(preamble_columns+static_cast<unsigned int>(to_print.substr(0,newline_pos).size()));
                    text_ptr += newline_pos+1;
                } else { // Text reaches the end of the terminal line
                    std::clog << _apply_theme(to_print);
                    _cover_held_columns_with_whitespaces(preamble_columns+static_cast<unsigned int>(to_print.size()));
                    text_ptr += max_columns-preamble_columns;
                }
                std::clog << '\n';
                if (_is_holding()) _print_held_line();
                if (_is_holding()) std::clog << '\r';

                _print_preamble_for_extralines(msg.level,msg.identifier.size());
            } else { // (remaining) Text shorter than the terminal line
                std::string to_print = text.substr(text_ptr,text_size-text_ptr);
                std::size_t newline_pos = to_print.find('\n');
                if (newline_pos != std::string::npos) { // A newline is found before reaching the end of the terminal line
                    std::clog << _apply_theme(to_print.substr(0,newline_pos));
                    _cover_held_columns_with_whitespaces(preamble_columns+static_cast<unsigned int>(to_print.substr(0,newline_pos).size()));
                    std::clog << '\n';
                    if (_is_holding()) {
                        _print_held_line();
                        std::clog << '\r';
                    }

                    text_ptr += newline_pos+1;
                    _print_preamble_for_extralines(msg.level,msg.identifier.size());
                } else { // Text reaches the end of the terminal line
                    std::clog << _apply_theme(to_print);
                    _cover_held_columns_with_whitespaces(preamble_columns+static_cast<unsigned int>(to_print.size()));
                    std::clog << '\n';
                    if (_is_holding()) _print_held_line();

                    break;
                }
            }
        }
    } else { // No multiline is handled, \n characters are handled by the terminal
        std::clog << _apply_theme(text);
        _cover_held_columns_with_whitespaces(preamble_columns+static_cast<unsigned int>(text.size()));
        std::clog << '\n';
        if (_is_holding()) _print_held_line();
    }
    _cached_last_printed_level = msg.level;
    _cached_last_printed_thread_name = msg.identifier;
}

void Logger::_hold(LogRawMessage const& msg) {
    std::lock_guard<std::mutex> lock(_output_mutex);
    bool scope_found = false;
    for (unsigned int idx=0; idx<_current_held_stack.size(); ++idx) {
        if (_current_held_stack[idx].scope == msg.scope) { _current_held_stack[idx] = msg; scope_found = true; break; } }
    if (not scope_found) { _current_held_stack.push_back(msg); }
    _print_held_line();
}

void Logger::_release(LogRawMessage const& msg) {
    std::lock_guard<std::mutex> lock(_output_mutex);
    if (_is_holding()) {
        bool found = false;
        unsigned int i=0;
        for (; i<_current_held_stack.size(); ++i) {
            if (_current_held_stack[i].scope == msg.scope) {
                found = true;
                break;
            }
        }
        if (found) {
            auto released_text_length = static_cast<unsigned int>(_current_held_stack[i].text.size()+4);
            std::vector<LogRawMessage> new_held_stack;
            if (i>0) {
                for (unsigned int j=0; j<i; ++j) {
                    new_held_stack.push_back(_current_held_stack[j]);
                }
            }
            _current_held_stack = new_held_stack;
            _print_held_line(); // Re-print
            std::clog << std::string(std::min(released_text_length,get_window_columns()), ' '); // Fill the released chars with blanks
            if (not _is_holding()) // If nothing is held anymore, allow overwriting of the line
                std::clog << '\r';
            std::clog << std::flush;
        }
    }
}


} // namespace Ariadne

inline bool startup_logging() {
    std::cerr << std::boolalpha;
    std::cout << std::boolalpha;
    std::clog << std::boolalpha;
    return true;
}

static const bool startup_logging_ = startup_logging();

