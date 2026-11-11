/***************************************************************************
 *            tests/thread.hpp
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

/** @file tests/thread.hpp
 *  @brief Thread helpers used by the Logging self-tests.
 */

#ifndef LOGGING_THREAD_HPP
#define LOGGING_THREAD_HPP

#include <atomic>
#include <functional>
#include <future>
#include <mutex>
#include <thread>
#include <utility>

#include "logging/logging.hpp"

namespace Ariadne {

/** @brief Return the string representation. */
template<class T>
inline std::string to_string(const T& t) {
    std::stringstream ss;
    ss << t;
    return ss.str();
}

//! \brief A class for handling a thread for a pool in a smarter way.
//! \details It allows to wait for the start of the \a task before extracting the thread id, which is held along with
//! a readable \a name.
class Thread {
  public:
    //! \brief Construct with an optional name
    //! \details The thread will start and store the id
    Thread(const std::function<void()>& task, const std::string& name = std::string())
        : _name(name), _got_id_future(_got_id_promise.get_future()), _registered_thread_future(_registered_thread_promise.get_future()) {
        _thread = std::thread([this, task]() {
            _id = std::this_thread::get_id();
            _got_id_promise.set_value();
            _registered_thread_future.get();
            task();
        });
        _got_id_future.get();
        if (_name.empty())
            _name = to_string(_id);
        Logger::instance().register_thread(this->id(), this->name());
        _registered_thread_promise.set_value();
    }

    //! \brief Get the thread id
    const std::thread::id& id() const { return _id; }
    //! \brief Get the readable name
    const std::string& name() const { return _name; }

    //! \brief Destroy the instance
    ~Thread() {
        _thread.join();
        Logger::instance().unregister_thread(this->id());
    }

  private:
    std::string _name;
    std::thread::id _id;
    std::promise<void> _got_id_promise;
    std::future<void> _got_id_future;
    std::promise<void> _registered_thread_promise;
    std::future<void> _registered_thread_future;
    std::thread _thread;
};

} // namespace Ariadne

#endif // LOGGING_THREAD_HPP
