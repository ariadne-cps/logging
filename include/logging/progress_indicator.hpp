/***************************************************************************
 *            logging/progress_indicator.hpp
 *
 *  Copyright  2021  Luca Geretti
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

#ifndef ARIADNE_LOGGING_PROGRESS_INDICATOR_HPP
#define ARIADNE_LOGGING_PROGRESS_INDICATOR_HPP

#include <cmath>

namespace Ariadne {

//! \brief Helper class to display progress percentage when holding text in Logger
class ProgressIndicator {
  public:
    ProgressIndicator(double final_value = 0.0) : _final_value(final_value), _current_value(0u), _step(0u) {}
    double final_value() const { return _final_value; }
    double current_value() const { return _current_value; }
    void update_current(double new_value) {
        if (_current_value != new_value) {
            _current_value = new_value;
            ++_step;
        }
    }
    void update_final(double new_final) { _final_value = new_final; }
    char symbol() const {
        switch (_step % 4) {
            case 0:
                return '\\';
            case 1:
                return '|';
            case 2:
                return '/';
            default:
                return '-';
        }
    }
    unsigned int percentage() const {
        // An unset or zero-length interval has no measurable progress.
        // Never convert NaN, infinity, or an out-of-range value to unsigned.
        if (!std::isfinite(_final_value) || !std::isfinite(_current_value) || _final_value <= 0.0) {
            return 0u;
        }
        const double fraction = _current_value / _final_value;
        if (fraction <= 0.0)
            return 0u;
        if (fraction >= 1.0)
            return 100u;
        return static_cast<unsigned int>(std::round(fraction * 100.0));
    }

  private:
    double _final_value;
    double _current_value;
    unsigned int _step;
};

} // namespace Ariadne

#endif // ARIADNE_LOGGING_PROGRESS_INDICATOR_HPP
