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

#include "pages.hpp"
#include "config.hpp"
#include "melanobot/melanobot.hpp"
#include "formatter_html.hpp"

using namespace httpony::quick_xml;
using namespace httpony::quick_xml::html;

namespace web {

static melanobot::Melanobot& bot()
{
    return melanobot::Melanobot::instance();
}

std::shared_ptr<BlockElement> page_link(
    const Request& request,
    const UriPath& path,
    const Text& text)
{
    if ( UriPathSlice(request.uri.path).match_exactly(path) )
        return std::make_shared<Element>("span", text, Attribute("class", "current_page"));
    else
        return std::make_shared<Link>(path.url_encoded(true), text);
}

class ServiceStatus
{
public:
    ServiceStatus(network::Connection::Status status)
    {
        if ( status > network::Connection::CHECKING )
        {
            status_name = "Connected";
            short_name = "OK";
        }
        else if ( status >= network::Connection::CONNECTING )
        {
            status_name = "Connecting";
            short_name = "...";
        }
        else
        {
            status_name = "Disconnected";
            short_name = "(!)";
        }
    }

    ServiceStatus(bool status)
        : ServiceStatus(status ? network::Connection::CONNECTED : network::Connection::DISCONNECTED)
        {}

    const std::string& name() const
    {
        return status_name;
    }

    Attribute css_class() const
    {
        return Attribute("class", "status_" + melanolib::string::strtolower(status_name));
    }

    httpony::quick_xml::BlockElement element(const std::string& tag = "span") const
    {
        return BlockElement{tag, css_class(), Text(status_name)};
    }

    httpony::quick_xml::BlockElement short_element(const std::string& tag = "span") const
    {
        return BlockElement{tag, css_class(), Text(short_name)};
    }


private:
    std::string status_name;
    std::string short_name;
};

static List connection_list(const Request& request, const UriPath& base_path)
{
    List connections;
    for ( const auto& conn : bot().connection_names() )
    {
        auto link = page_link(request, base_path/"connection"/conn, conn);
        link->append(ServiceStatus(bot().connection(conn)->status()).short_element());
        connections.add_item(link);
    }
    return connections;
}

static List service_list(const Request& request, const UriPath& base_path)
{
    List services;
    for ( const auto& svc : bot().service_list() )
    {
        auto link = page_link(
            request,
            base_path/"service"/std::to_string(uintptr_t(svc.get())),
            svc->name()
        );
        link->append(ServiceStatus(svc->running()).short_element());
        services.add_item(link);
    }
    return services;
}

static void flatten_tree(
    const PropertyTree& tree,
    const std::string& prefix,
    httpony::quick_xml::html::Table& table)
{
    for ( const auto& prop : tree)
    {
        table.add_data_row(Text(prop.first), Text(prop.second.data()));
        flatten_tree(prop.second, prefix + prop.first + '.', table);
    }
}

class StatusPage::SubPage
{
public:
    using BlockElement = httpony::quick_xml::BlockElement;
    using PathSuffix = UriPathSlice;

    SubPage(std::string name, UriPath path)
        : _name(std::move(name)), _path(std::move(path))
    {}

    virtual ~SubPage(){}

    virtual bool match_path(const PathSuffix& path) const
    {
        return path.match_prefix(_path);
    }

    const std::string& name() const
    {
        return _name;
    }

    const UriPath& path() const
    {
        return _path;
    }

    virtual void render(
        Request& request,
        const PathSuffix& path,
        BlockElement& parent,
        const UriPath& link_base_path
    ) const = 0;

    virtual bool has_submenu() const
    {
        return false;
    }

    virtual List submenu(const Request& request, const UriPath& link_base_path) const
    {
        return List();
    }

private:
    std::string _name;
    UriPath _path;
};

class Home : public StatusPage::SubPage
{
public:
    Home() : SubPage("Home", "") {}

    bool match_path(const PathSuffix& path) const
    {
        return path.empty();
    }

    void render(
        Request& request,
        const PathSuffix& path,
        BlockElement& parent,
        const UriPath& link_base_path
    ) const override
    {
        parent.append(Element{"h1", Text{PROJECT_NAME}});
        parent.append(Element{"h2", Text{"Build"}});
        Table table;
        table.add_header_row(Text{"Property"}, Text{"Value"});
        table.add_data_row(Text{"Name"}, Text{PROJECT_NAME});
        table.add_data_row(Text{"Version"}, Text{PROJECT_DEV_VERSION});
        table.add_data_row(Text{"OS"}, Text{SYSTEM_NAME " " SYSTEM_VERSION});
        table.add_data_row(Text{"Processor"}, Text{SYSTEM_PROCESSOR});
        table.add_data_row(Text{"Compiler"}, Text{SYSTEM_COMPILER});
        parent.append(table);
        parent.append(Element{"h2", Text{"Connections"}});
        parent.append(connection_list(request, link_base_path));
        parent.append(Element{"h2", Text{"Services"}});
        parent.append(service_list(request, link_base_path));

    }
};

class Connections : public StatusPage::SubPage
{
public:
    Connections() : SubPage("Connections", "connection") {}

    void render(
        Request& request,
        const PathSuffix& path,
        BlockElement& parent,
        const UriPath& link_base_path
    ) const override
    {
        if ( path.size() == 0 )
        {
            parent.append(Element{"h1", Text{"Connections"}});
            parent.append(connection_list(request, link_base_path));
            return;
        }

        if ( path.size() != 1 )
            throw HttpError(StatusCode::NotFound);

        network::Connection* conn = bot().connection(path[0]);
        if ( !conn )
            throw HttpError(StatusCode::NotFound);

        parent.append(Element{"h1", Text{path[0]}});

        Table table;
        table.add_header_row(Text{"Property"}, Text{"Value"});
        table.add_data_row(Text("Protocol"), Text(conn->protocol()));
        table.add_data_row(Text("Status"), ServiceStatus(conn->status()).element());
        table.add_data_row(Text("Name"), Text(conn->name()));
        table.add_data_row(Text("Config Name"), Text(conn->config_name()));
        table.add_data_row(Text("Formatter"), Text(conn->formatter()->name()));
        table.add_data_row(Text("Server"), Text(conn->server().name()));

        auto pretty = conn->pretty_properties();
        if ( !pretty.empty() )
        {
            table.add_row(Element("th", Attribute("colspan", "2"), Text("Formatting")));
            FormatterHtml formatter;
            for ( const auto& prop : pretty )
                table.add_data_row(Text(prop.first), Text(prop.second.encode(formatter)));
        }

        auto internal = conn->properties().copy();
        if ( !internal.empty() )
        {
            table.add_row(Element("th", Attribute("colspan", "2"), Text("Internal")));
            flatten_tree(internal, "", table);
        }

        parent.append(table);
    }

    virtual bool has_submenu() const override
    {
        return true;
    }

    virtual List submenu(const Request& request, const UriPath& link_base_path) const override
    {
        return connection_list(request, link_base_path);
    }
};

class Services : public StatusPage::SubPage
{
public:
    Services() : SubPage("Services", "service") {}

    void render(
        Request& request,
        const PathSuffix& path,
        BlockElement& parent,
        const UriPath& link_base_path
    ) const override
    {
        if ( path.size() == 0 )
        {
            parent.append(Element{"h1", Text{"Services"}});
            parent.append(service_list(request, link_base_path));
            return;
        }

        if ( path.size() != 1 )
            throw HttpError(StatusCode::NotFound);

        AsyncService* service = nullptr;
        for ( const auto& svc : bot().service_list() )
            if ( std::to_string(uintptr_t(svc.get())) == path[0] )
            {
                service = svc.get();
                break;
            }
        if ( !service )
            throw HttpError(StatusCode::NotFound);

        parent.append(Element{"h1", Text{service->name()}});

        Table table;
        table.add_header_row(Text{"Property"}, Text{"Value"});
        table.add_data_row(Text("Status"), ServiceStatus(service->running()).element());
        table.add_data_row(Text("Name"), Text(service->name()));

        parent.append(table);
    }

    virtual bool has_submenu() const override
    {
        return true;
    }

    virtual List submenu(const Request& request, const UriPath& link_base_path) const override
    {
        return service_list(request, link_base_path);
    }
};

class GlobalSettings : public StatusPage::SubPage
{
public:
    GlobalSettings() : SubPage("Global Settings", "settings") {}

    void render(
        Request& request,
        const PathSuffix& path,
        BlockElement& parent,
        const UriPath& link_base_path
    ) const override
    {
        if ( !path.empty() )
            throw HttpError(StatusCode::NotFound);

        parent.append(Element{"h1", Text{"Global Settings"}});

        Table table;
        table.add_header_row(Text{"Property"}, Text{"Value"});
        flatten_tree(settings::global_settings, "", table);
        parent.append(table);
    }
};

StatusPage::StatusPage(const Settings& settings)
{
    uri = read_uri(settings, "");
    css_file = settings.get("css", css_file);
    sub_pages.push_back(melanolib::New<Home>());
    sub_pages.push_back(melanolib::New<GlobalSettings>());
    sub_pages.push_back(melanolib::New<Connections>());
    sub_pages.push_back(melanolib::New<Services>());
}

StatusPage::~StatusPage() = default;

Response StatusPage::respond(Request& request, const UriPathSlice& path, const HttpServer& sv) const
{
    auto local_path = path.left_stripped(uri.size());
    HtmlDocument html("Bot status");
    UriPath base_path = local_path.strip_path_suffix(request.uri.path).to_path();

    if ( !css_file.empty() )
    {
        html.head().append(Element("link", Attributes{
            {"rel", "stylesheet"}, {"type", "text/css"}, {"href", css_file}
        }));
    }

    List menu;
    menu.append(Attribute("class", "menu"));
    SubPage* current_page = nullptr;
    for ( const auto& page : sub_pages )
    {
        if ( page->match_path(local_path) )
            current_page = page.get();

        menu.add_item(page_link(request, base_path + page->path(), page->name()));
    }
    if ( !current_page )
        throw HttpError(StatusCode::NotFound);

    Element nav("nav");
    nav.append(menu);

    if ( current_page->has_submenu() )
    {
        List submenu = current_page->submenu(request, base_path);
        submenu.append(Attribute("class", "submenu"));
        nav.append(submenu);
    }

    html.body().append(nav);

    BlockElement contents("div", Attribute("class", "contents"));
    current_page->render(
        request,
        local_path.left_stripped(current_page->path().size()),
        contents,
        base_path
    );
    html.body().append(contents);

    Response response("text/html", StatusCode::OK, request.protocol);
    html.print(response.body, true);
    return response;
}
} // namespace web
