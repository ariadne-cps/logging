/***************************************************************************
 *            logging_command_line_interface.cpp
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
 *
 *  Logging is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Logging.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "logging/logging_command_line_interface.hpp"
#include "logging/logging.hpp"

namespace Ariadne {

namespace {

constexpr Int SCHEDULER_PRIORITY = -100;

ArgumentParser make_scheduler_argument_parser() {
    return make_valued_argument_parser(
        "s",
        "scheduler",
        "Choose the logging scheduler as a <value> in [ immediate | blocking | nonblocking ] (default: nonblocking)",
        [](String const& value) -> VoidFunction {
            if (value == "immediate") {
                return [] { Logger::instance().use_immediate_scheduler(); };
            }
            if (value == "blocking") {
                return [] { Logger::instance().use_blocking_scheduler(); };
            }
            if (value == "nonblocking") {
                return [] { Logger::instance().use_nonblocking_scheduler(); };
            }
            throw std::runtime_error("invalid logging scheduler");
        },
        SCHEDULER_PRIORITY);
}

ArgumentParser make_theme_argument_parser() {
    return make_valued_argument_parser(
        "t",
        "theme",
        "Choose the logging theme as a <value> in [ none | light | dark ] (default: none)",
        [](String const& value) -> VoidFunction {
            TerminalTextTheme theme=TT_THEME_NONE;
            if (value != "none") {
                if (value == "light") {
                    theme=TT_THEME_LIGHT;
                } else if (value == "dark") {
                    theme=TT_THEME_DARK;
                } else {
                    throw std::runtime_error("invalid logging theme");
                }
            }
            return [theme] {
                Logger::instance().configuration().set_theme(theme);
            };
        });
}

ArgumentParser make_verbosity_argument_parser() {
    return make_valued_argument_parser(
        "v",
        "verbosity",
        "Choose the logging verbosity as a non-negative integer <value> (default: 0)",
        [](String const& value) -> VoidFunction {
            Int const verbosity=std::stoi(value);
            if (verbosity < 0) {
                throw std::runtime_error("negative logging verbosity");
            }
            return [verbosity] {
                Logger::instance().configuration().set_verbosity(
                    static_cast<unsigned int>(verbosity));
            };
        });
}

} // namespace

LoggingCommandLineInterface::LoggingCommandLineInterface() {
    add(CommandLineContribution(
        "logging",
        {
            make_scheduler_argument_parser(),
            make_theme_argument_parser(),
            make_verbosity_argument_parser()
        }));
}

} // namespace Ariadne
