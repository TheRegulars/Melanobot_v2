/**
 * \file
 * \author Mattia Basaglia
 * \copyright Copyright 2015-2016 Mattia Basaglia
 * \section License
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "handler.hpp"

namespace handler {

bool SimpleAction::handle(network::Message& msg)
{
    if ( can_handle(msg) )
    {
        if ( trigger.empty() )
            return on_handle(msg);

        std::smatch match;
        if ( matches_pattern(msg, match) )
        {
            std::string old = msg.message;
            msg.message.erase(0, match.length());
            auto ret = on_handle(msg);
            msg.message = old;
            return ret;
        }
    }
    return false;
}

} // namespace handler
