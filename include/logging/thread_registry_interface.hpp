/***************************************************************************
 *            logging/thread_registry_interface.hpp
 *
 *  Copyright  2022  Luca Geretti
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

/** @file logging/thread_registry_interface.hpp
 *  @brief Interface for querying whether threads are registered.
 */

#ifndef ARIADNE_LOGGING_THREAD_REGISTRY_INTERFACE_HPP
#define ARIADNE_LOGGING_THREAD_REGISTRY_INTERFACE_HPP

namespace Ariadne {

//! \brief Interface to query the presence of registered threads
class ThreadRegistryInterface {
  public:
    //! \brief Check whether there are threads registered
    virtual bool has_threads_registered() const = 0;
};

} // namespace Ariadne

#endif // ARIADNE_LOGGING_THREAD_REGISTRY_INTERFACE_HPP
