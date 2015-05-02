/**
 * \file
 * \author Mattia Basaglia
 * \copyright Copyright 2015 Mattia Basaglia
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
#ifndef XONOTIC_HANDLER_STATUS_HPP
#define XONOTIC_HANDLER_STATUS_HPP

#include "handler/handler.hpp"
#include "string/string_functions.hpp"
#include "xonotic/xonotic.hpp"

namespace xonotic {

/**
 * \brief Returns a list of properties to be used for message formatting
 */
inline Properties message_properties(network::Connection* source)
{
    string::FormatterConfig fmt;
    user::UserCounter count = source->count_users();
    return Properties {
        {"players", std::to_string(count.users)},
        {"bots",    std::to_string(count.bots)},
        {"total",   std::to_string(count.users+count.bots)},
        {"max",     std::to_string(count.max)},
        {"free",    std::to_string(count.max-count.users)},
        {"map",     source->get_property("map")}, /// \todo replace with "?" or something
        {"gt",      source->get_property("gametype")}, /// same here
        {"gametype",xonotic::gametype_name(source->get_property("gametype"))},
        {"sv_host", source->formatter()->decode(source->get_property("host")).encode(&fmt)},
        {"sv_server",source->server().name()}
    };
}

inline Properties message_properties(network::Connection* source,
                                     const user::User& user)
{
    string::FormatterConfig fmt;
    Properties props =  message_properties(source);
    props.insert({
        {"name",    source->formatter()->decode(user.name).encode(&fmt)},
        {"ip",      user.host},
    });
    return props;
}

/**
 * \brief Show server connect/disconnect messages
 */
class ConnectionEvents : public handler::Handler
{
public:
    ConnectionEvents( const Settings& settings, HandlerContainer* parent )
        : Handler(settings, parent)
    {
        connect = settings.get("connect","Server #2#%host#-# connected.");
        disconnect = settings.get("connect","#-b#Warning!#-# Server #1#%host#-# disconnected.");
    }

    bool can_handle(const network::Message& msg) const override
    {
        return string::is_one_of(msg.command,{"CONNECTED","DISCONNECTED"});
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        string::FormatterConfig fmt;
        /// \todo Should the host property be returned from description()?
        Properties props = {
            {"host",msg.source->formatter()->decode(msg.source->get_property("host")).encode(&fmt)},
            {"server",msg.source->server().name()}
        };
        const std::string& str = msg.command == "CONNECTED" ? connect : disconnect;
        reply_to(msg,fmt.decode(string::replace(str,props,"%")));
        return true;
    }

private:
    std::string connect;
    std::string disconnect;
};

/**
 * \brief Shows player join/part messages
 */
class XonoticJoinPart : public handler::Handler
{
public:
    XonoticJoinPart( const Settings& settings, HandlerContainer* parent )
        : Handler(settings, parent)
    {
        join = settings.get("join",join);
        part = settings.get("part",part);
        bots = settings.get("bots",bots);
    }

    bool can_handle(const network::Message& msg) const override
    {
        return ( msg.type == network::Message::JOIN ||
                 msg.type == network::Message::PART ) &&
               ( bots || !msg.from.host.empty() );
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        string::FormatterConfig fmt;
        Properties props = msg.from.properties;
        auto overprops = message_properties(msg.source,msg.from);
        props.insert(overprops.begin(),overprops.end());

        const std::string& message = msg.type == network::Message::JOIN ? join : part;
        reply_to(msg,fmt.decode(string::replace(message,props,"%")));
        return true;
    }

private:
    std::string join = "#2#+ join#-#: %name #1#%map#-# [#1#%players#-#/#1#%max#-#]";
    std::string part = "#1#- part#-#: %name #1#%map#-# [#1#%players#-#/#1#%max#-#]";
    bool        bots = false;
};

/**
 * \brief Shows match start messages
 */
class XonoticMatchStart : public handler::Handler
{
public:
    XonoticMatchStart( const Settings& settings, HandlerContainer* parent )
        : Handler(settings, parent)
    {
        message = settings.get("message",message);
        empty = settings.get("empty",empty);
    }

    bool can_handle(const network::Message& msg) const override
    {
        return msg.command == "gamestart";
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        string::FormatterConfig fmt;
        Properties props = message_properties(msg.source);

        if ( empty || msg.source->count_users().users )
            reply_to(msg,fmt.decode(string::replace(message,props,"%")));
        return true;
    }

private:
    std::string message = "Playing #dark_cyan#%gametype#-# on #1#%map#-# (%free free slots); join now: #-b#xonotic +connect %sv_server";
    bool empty = false;
};

/**
 * \brief Base class for handlers parsing the Xonotic eventlog
 *
 * It defines a new on_handle which accepts a regex_result
 */
class ParseEventlog : public handler::Handler
{
public:
    ParseEventlog(std::string regex, const Settings& settings, HandlerContainer* parent)
        : Handler(settings, parent),
        regex(std::move(regex), std::regex::ECMAScript|std::regex::optimize)
    {}

    bool can_handle(const network::Message& msg) const override
    {
        return msg.command == "n";
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        std::smatch match;
        if ( std::regex_match(msg.raw,match,regex) )
        {
            return on_handle(msg,match);
        }
        return false;
    }

    virtual bool on_handle(network::Message& msg, const std::smatch& match) = 0;

private:
    std::regex regex;
};

/**
 * \brief Shows :vote:vcall
 */
class ShowVoteCall : public ParseEventlog
{
public:
    ShowVoteCall(const Settings& settings, HandlerContainer* parent)
    : ParseEventlog(R"(^:vote:vcall:(\d+):(.*))",settings,parent)
    {
        message = settings.get("message",message);
    }

protected:
    bool on_handle(network::Message& msg, const std::smatch& match) override
    {
        string::FormatterConfig fmt;
        user::User user = msg.source->get_user(match[1]);
        Properties props = message_properties(msg.source,user);
        props["vote"] = msg.source->formatter()->decode(match[2]).encode(fmt);
        reply_to(msg,fmt.decode(string::replace(message,props,"%")));
        return true;
    }

private:
    std::string message = "#4#*#-# %name#-# calls a vote for %vote";
};

/**
 * \brief Shows :vote:v(yes|no|timeout)
 */
class ShowVoteResult : public ParseEventlog
{
public:
    ShowVoteResult(const Settings& settings, HandlerContainer* parent)
    : ParseEventlog(R"(^:vote:v(yes|no|timeout):(\d+):(\d+):(\d+):(\d+):(-?\d+))",settings,parent)
    {
        message = settings.get("message",message);
        message_yes = settings.get("message_yes",message_yes);
        message_no = settings.get("message_no",message_no);
        message_timeout = settings.get("message_timeout",message_timeout);
        message_abstain = settings.get("message_abstain",message_abstain);
        message_min = settings.get("message_min",message_min);
    }

protected:
    bool on_handle(network::Message& msg, const std::smatch& match) override
    {
        string::FormatterConfig fmt;
        Properties props = message_properties(msg.source);
        props["result"] = match[1];
        props["yes"] = match[2];
        props["no"] = match[3];
        props["abstain"] = match[4];
        props["novote"] = match[5];
        int abstain_total = std::stoi(match[4])+std::stoi(match[5]);
        props["abstain_total"] = std::to_string(abstain_total);
        props["min"] = match[6];
        int min = std::stoi(match[6]);

        if ( props["result"] == "yes" )
            props["message_result"] = string::replace(message_yes,props,"%");
        else if ( props["result"] == "no" )
            props["message_result"] = string::replace(message_no,props,"%");
        else
            props["message_result"] = string::replace(message_timeout,props,"%");

        props["message_abstain"] = abstain_total <= 0 ? "" :
            string::replace(message_abstain,props,"%");

        props["message_min"] = min <= 0 ? "" :
            string::replace(message_min,props,"%");

        reply_to(msg,fmt.decode(string::replace(message,props,"%")));
        return true;
    }

private:
    /// Full message
    std::string message         = "#4#*#-# vote %message_result: #dark_green#%yes#-#:#1#%no#-#%message_abstain%message_min";
    /// Message shown when the result is yes
    std::string message_yes     = "#dark_green#passed";
    /// Message shown when the result is no
    std::string message_no      = "#1#failed";
    /// Message shown when the result is timeout
    std::string message_timeout = "#dark_yellow#timed out";
    /// Message shown when the %abstain_total is greater than zero
    std::string message_abstain = ", %abstain_total didn't vote";
    /// Message shown when %min is greater than zero
    std::string message_min     = " (%min needed)";
};

/**
 * \brief Shows :vote:vstop
 */
class ShowVoteStop : public ParseEventlog
{
public:
    ShowVoteStop(const Settings& settings, HandlerContainer* parent)
    : ParseEventlog(R"(^:vote:vstop:(\d+))",settings,parent)
    {
        message = settings.get("message",message);
    }

protected:
    bool on_handle(network::Message& msg, const std::smatch& match) override
    {
        string::FormatterConfig fmt;
        user::User user = msg.source->get_user(match[1]);
        Properties props = message_properties(msg.source,user);
        reply_to(msg,fmt.decode(string::replace(message,props,"%")));
        return true;
    }

private:
    std::string message = "#4#*#-# %name#-# stopped the vote";
};

/**
 * \brief Shows :vote:vlogin
 */
class ShowVoteLogin : public ParseEventlog
{
public:
    ShowVoteLogin(const Settings& settings, HandlerContainer* parent)
    : ParseEventlog(R"(^:vote:vlogin:(\d+))",settings,parent)
    {
        message = settings.get("message",message);
    }

protected:
    bool on_handle(network::Message& msg, const std::smatch& match) override
    {
        string::FormatterConfig fmt;
        user::User user = msg.source->get_user(match[1]);
        Properties props = message_properties(msg.source,user);
        reply_to(msg,fmt.decode(string::replace(message,props,"%")));
        return true;
    }

private:
    std::string message = "#4#*#-# %name#-# logged in as #dark_yellow#master";
};

/**
 * \brief Shows :vote:vdo
 */
class ShowVoteDo : public ParseEventlog
{
public:
    ShowVoteDo(const Settings& settings, HandlerContainer* parent)
    : ParseEventlog(R"(^:vote:vdo:(\d+):(.*))",settings,parent)
    {
        message = settings.get("message",message);
    }

protected:
    bool on_handle(network::Message& msg, const std::smatch& match) override
    {
        string::FormatterConfig fmt;
        user::User user = msg.source->get_user(match[1]);
        Properties props = message_properties(msg.source,user);
        props["vote"] = msg.source->formatter()->decode(match[2]).encode(fmt);
        reply_to(msg,fmt.decode(string::replace(message,props,"%")));
        return true;
    }

private:
    std::string message = "#4#*#-# %name#-# used their master status to do %vote";
};

/**
 * \brief Shows match scores
 * \todo Could use parameterized score list messages
 */
class XonoticMatchScore : public ParseEventlog
{
private:
    /**
     * \brief Struct for player score
     */
    struct PlayerScore
    {
        std::string name;       ///< Player name
        int score = 0;          ///< Score
        int id = 0;             ///< Player ID
        PlayerScore(std::string name, int score, int id)
            : name(std::move(name)), score(score), id(id) {}

        bool operator< (const PlayerScore& o) const noexcept { return score < o.score; }
        bool operator> (const PlayerScore& o) const noexcept { return score > o.score; }
    };

public:
    XonoticMatchScore( const Settings& settings, HandlerContainer* parent )
        : ParseEventlog(
            ":(?:"
            // 1
            "(end)"
            // 2                     score=3                id=4
            "|(teamscores:see-labels:(-?\\d+)(?:-|[0-9,])*:(\\d+))"
            // 5                  score=6              time=7 team=8   id=9 name=10
            "|(player:see-labels:(-?\\d+)(?:-|[0-9,])*:(\\d+):([^:]+):(\\d+):(.*))"
            // 11   gametype=12 map=13
            "|(scores:([a-z]+)_(.*)):\\d+"
            // 14           primary=15 sort=16
            "|(labels:player:([^[,<!]*)(<)?!!,.*)"
            ")",
            settings, parent
        )
    {
        message = settings.get("message",message);
        empty = settings.get("empty",empty);
    }

    /**
     * \todo maybe this check isn't needed (empty check can be deferred to :end)
     */
    bool can_handle(const network::Message& msg) const override
    {
        return !msg.raw.empty() && msg.raw[0] == ':' &&
            (empty || msg.source->count_users().users);
    }

protected:
    bool on_handle(network::Message& msg, const std::smatch& match) override
    {
        if ( match[1].matched )
            handle_end(msg,match);
        else if ( match[2].matched )
            handle_team(match);
        else if ( match[5].matched )
            handle_player(msg.source,match);
        else if ( match[11].matched )
            handle_scores(msg.source,match);
        else if ( match[14].matched )
            handle_labels(match);

        return true;
    }

    /**
     * \brief Handles :end and shows the score list
     */
    void handle_end(const network::Message& msg, const std::smatch& match)
    {
        string::FormatterConfig fmt;
        Properties props = message_properties(msg.source);

        reply_to(msg,fmt.decode(string::replace(message,props,"%")));

        if ( !player_scores.empty() || !team_scores.empty() )
            print_scores(msg);

        player_scores.clear();
        team_scores.clear();
        sort_reverse = false;
    }

    /**
     * \brief Prints the score for all the players in a given team
     */
    void print_scores(const network::Message& msg, int team)
    {
        auto it = player_scores.find(team);
        if ( it == player_scores.end() || it->second.empty() )
            return;

        if ( team != SPECTATORS )
        {
            if ( sort_reverse )
                std::stable_sort(it->second.begin(),it->second.end());
            else
                std::stable_sort(it->second.rbegin(),it->second.rend());
        }

        string::FormattedString out;
        auto color = team_colors[team];

        auto itt = team_scores.find(team);
        if ( itt != team_scores.end() )
            out << color << itt->second << color::nocolor << ") ";

        for ( unsigned i = 0; i < it->second.size(); i++ )
        {
            if ( team != SPECTATORS )
                out << color << string::FormatFlags::BOLD << it->second[i].score
                    << string::ClearFormatting() << ' ';

            out << msg.source->formatter()->decode(it->second[i].name)
                << string::ClearFormatting();

            if ( i < it->second.size() - 1 )
                out << ", ";
        }

        reply_to(msg, out);
    }

    /**
     * \brief Prints the score list
     */
    void print_scores(const network::Message& msg)
    {
        if ( !show_spectators && player_scores.size() < 2 )
            return;

        auto it = player_scores.begin();
        for ( ++it; it != player_scores.end(); ++it )
            print_scores(msg,it->first);

        if ( show_spectators )
            print_scores(msg,SPECTATORS);
    }

    /**
     * \brief Handles :teamscores:see-labels and gathers the team scores
     */
    void handle_team(const std::smatch& match)
    {
        team_scores[string::to_int(match[4])] = string::to_int(match[3]);
    }

    /**
     * \brief Handles :player:see-labels and gathers the player scores
     */
    void handle_player(network::Connection* conn, const std::smatch& match)
    {
        // Team number (-1 = no team, -2 = spectator)
        int team = string::to_int(match[8],10,SPECTATORS);
        int score = string::to_int(match[6]);
        if ( team == -1 && score == 0 && conn->get_property("gametype") == "lms" )
            team = SPECTATORS;
        player_scores[team].emplace_back(
            match[10],                     // name
            score,
            string::to_int(match[9])       // id
        );
    }

    /**
     * \brief Handles :scores, updates gametype and map and initializes the data structures
     */
    void handle_scores(network::Connection* conn, const std::smatch& match)
    {
        conn->set_property("gametype",match[12]);
        conn->set_property("map",match[13]);
        player_scores = { {SPECTATORS, {}} };
        team_scores.clear();
        sort_reverse = false;
    }

    /**
     * \brief Handles :labels:player and selects the sorting order
     */
    void handle_labels(const std::smatch& match)
    {
        sort_reverse = match[16].matched;
    }

private:
    bool empty = false;         ///< Whether to show the scores on an empty/bot-only match
    bool show_spectators = true; ///< Whether to show spectators on the score list
    std::string message = "#dark_cyan#%gametype#-# on #1#%map#-# ended"; ///< Message starting the score list

    std::map<int,std::vector<PlayerScore>>player_scores;  ///< Player Scores (team number => scores)
    std::map<int,int>                     team_scores;    ///< Team Scores (number => score)
    bool                                  sort_reverse{0};///< Whether a low score is better

    std::unordered_map<int, color::Color12> team_colors {
        {  5, color::red    },
        { 14, color::blue   },
        { 13, color::yellow },
        { 10, color::magenta}
    };

    enum { SPECTATORS = -2 };
};


} // namespace xonotic
#endif // XONOTIC_HANDLER_STATUS_HPP
