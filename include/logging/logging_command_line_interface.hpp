/***************************************************************************
 *            logging/logging_command_line_interface.hpp
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

/** @file logging/logging_command_line_interface.hpp
 *  @brief Command-line options for configuring the logging module.
 */

#ifndef ARIADNE_LOGGING_COMMAND_LINE_INTERFACE_HPP
#define ARIADNE_LOGGING_COMMAND_LINE_INTERFACE_HPP

#include "utility/command_line_interface.hpp"

namespace Ariadne {

//! \brief Command-line interface configuring the logging module.
//! \details Adds scheduler, theme and verbosity options and allows derived
//! interfaces to extend the profile using add().
class LoggingCommandLineInterface : public CommandLineInterface {
  public:
    /** @brief Construct a \c LoggingCommandLineInterface with the specified characteristics. */
    LoggingCommandLineInterface();
};

} // namespace Ariadne

#endif // ARIADNE_LOGGING_COMMAND_LINE_INTERFACE_HPP
