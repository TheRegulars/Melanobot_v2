/**
 * \file
 * \author Mattia Basaglia
 * \copyright Copyright 2015-2017 Mattia Basaglia
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
#ifndef USER_HPP
#define USER_HPP

#include <string>
#include <vector>
#include <unordered_map>

#include "settings.hpp"

namespace network {
    class Connection;
} // namespace network

/**
 * \brief Namespace for user handling utilities
 */
namespace user {

/**
 * \brief A user visible from a connection
 * \todo don't know if it might be worth avoiding direct member access
 *      things like \c name, \c host, \c global_id and \c properties can be
 *      modified without any issue, changing \c local_id might cause issues with
 *      the user manager but let's assume the Connection is able to keep them
 *      unique via protocol restrictions. Adding values to \c channels should
 *      be done using \c add_channel().
 *
 */
struct User
{
    User() = default;
    User ( std::string name,
           std::string host = "",
           std::string local_id = "",
           std::string global_id = "",
           network::Connection* origin = nullptr
         )
        : name(std::move(name)), host(std::move(host)),
          local_id(std::move(local_id)), global_id(std::move(global_id)),
          origin(origin)
    {}

    /**
     * \brief Checks if a user matches this one
     *
     * In order of priority, it will look for: \c global_id, \c host, \c local_id \c name.
     * The first of those to be non-empty on \b this, will be used to check the match
     */
    bool matches ( const User& user ) const
    {
        if ( !global_id.empty() )
            return user.global_id == global_id;
        if ( !host.empty() )
            return user.host == host;
        if ( !local_id.empty() )
            return user.local_id == local_id;
        return user.name == name;
    }

    /**
     * \brief Returns the value of that property or an empty string if not found
     */
    std::string property ( const std::string& name) const
    {
        auto it = properties.find(name);
        if ( it != properties.end() )
            return it->second;
        return {};
    }

    /**
     * \brief Add a channel (if not already in \c channels)
     */
    void add_channel(const std::string& channel)
    {
        for ( const auto& c: channels )
            if ( c == channel )
                return;
        channels.push_back(channel);
    }
    
    /**
     * \brief Remove a channel
     */
    void remove_channel(const std::string& channel)
    {
        for ( auto it = channels.begin(); it != channels.end(); ++it )
            if ( *it == channel )
            {
                channels.erase(it);
                return;
            }
    }

    /**
     * \brief Update attributes and properties from \c props
     */
    void update(const Properties& props)
    {
        for ( const auto& p : props )
        {
            auto it = string_attributes.find(p.first);
            if ( it != string_attributes.end() )
                this->*(it->second) = p.second;
            else
                properties[p.first] = p.second;
        }
    }

    /**
     * \brief User name
     *
     * It might have some custom formatting,
     * use the owner connection to get a formatted string
     */
    std::string name;

    /**
     * \brief Host name / IP address
     */
    std::string host;

    /**
     * \brief Unique id on the current connection
     */
    std::string local_id;

    /**
     * \brief Global id, if present can be used to uniquely identify a user
     */
    std::string global_id;

    /**
     * \brief List of channels this user is connected to
     */
    std::vector<std::string> channels;

    /**
     * \brief Custom properties associated to the user
     */
    Properties properties;

    /**
     * \brief Whether the user has been checked
     */
    bool checked = true;

    /**
     * \brief Connection the user belongs to
     */
    network::Connection* origin = nullptr;

private:
    /**
     * \brief Attributes that can be accessed as properties
     */
    static std::unordered_map<std::string, std::string User::*> string_attributes;
};

} // namespace user
#endif // USER_HPP
