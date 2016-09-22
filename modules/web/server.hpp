/**
 * \file
 *
 * \author Mattia Basaglia
 *
 * \copyright Copyright 2016 Mattia Basaglia
 *
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
#ifndef MELANOBOT_MODULE_WEB_SERVER_HPP
#define MELANOBOT_MODULE_WEB_SERVER_HPP

#include "base_pages.hpp"
#include "network/async_service.hpp"

namespace web {

/**
 * \brief HTTP server service
 */
class HttpServer : public AsyncService, public HttpRequestHandler,
                   public httpony::BasicPooledServer<httpony::ssl::SslServer>
{
    using ParentServer = httpony::BasicPooledServer<httpony::ssl::SslServer>;
public:
    HttpServer() : ParentServer(1, httpony::IPAddress{}) {}

    void initialize(const Settings& settings) override
    {
        httpony::IPAddress address(settings.get("address", ""));
        if ( auto port = settings.get_optional<uint16_t>("port") )
            address.port = *port;
        if ( auto ip_version = settings.get_optional<int>("ip_version") )
            address.type = *ip_version == 4 ?
                httpony::IPAddress::Type::IPv4 :
                httpony::IPAddress::Type::IPv6;
        set_listen_address(address);

        log_format = settings.get("log_format", log_format);

        /// \todo Allow configuring headers evaluated at runtime
        ///       (They could use the same syntax as the log format)
        for ( const auto& header : settings.get_child("Headers", {}) )
            headers.append(header.first, header.second.data());

        std::size_t threads = settings.get("threads", pool_size());
        if ( threads == 0 )
            throw melanobot::ConfigurationError("You need at least 1 thread");
        resize_pool(threads);

        /// \todo Read ssl config

        HttpRequestHandler::load_pages(settings.get_child("Pages", {}));
    }

    void start() override
    {
        ParentServer::start();
        Log("wsv", '!') << "Server started on port " << listen_address().port;
    }

    void stop() override
    {
        Log("wsv", '!') << "Stopping server on port " << listen_address().port;
        ParentServer::stop();
        Log("wsv", '!') << "Server stopped";
    }

    void respond(httpony::Request& request, const httpony::Status& status) override
    {
        auto response = HttpRequestHandler::respond(request, status,
                                                    request.uri.path, *this);

        for ( const auto& header : headers )
            if ( !response.headers.contains(header.first) )
                response.headers.append(header);

        if ( response.protocol >= httpony::Protocol::http_1_1 )
            response.headers["Connection"] = "close";
        
        response.clean_body(request);

        log_response(request, response);
        if ( !send(request.connection, response) )
            request.connection.close();
    }

    std::string format_info(const std::string& template_string,
                            const Request& request,
                            const Response& response) const
    {
        std::ostringstream stream;
        ParentServer::log_response(template_string, request, response, stream);
        return stream.str();
    }

protected:
    void error(httpony::io::Connection& connection, const httpony::OperationStatus& what) const override
    {
        ErrorLog("wsv") << "Server error: " << connection.remote_address() << ": " << what;
    }

private:
    void log_response(const Request& request, const Response& response) const
    {
        Log("wsv", '<') << format_info(log_format, request, response);
    }

    Response unhandled_error(const Request& request, const Status& status, const HttpServer& sv) const override
    {
        return ErrorPage::canned_response(status, request.protocol);
    }

    std::string log_format = "%h %l %u %t \"%r\" %s %b \"%{Referer}i\" \"%{User-Agent}i\"";
    httpony::Headers headers;
};

} // namespace web
#endif // MELANOBOT_MODULE_WEB_SERVER_HPP
