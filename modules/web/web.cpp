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
#include "web-api-concrete.hpp"
#include "http.hpp"
#include "melanomodule.hpp"

/**
 * \brief Initializes the web module
 */
Melanomodule melanomodule_web(const Settings&)
{
    Melanomodule module{"web","Web services"};
    module.register_log_type("web",color::dark_blue);
    module.register_service<network::http::HttpService>("web");

    module.register_handler<web::SearchVideoYoutube>("SearchVideoYoutube");
    module.register_handler<web::SearchImageGoogle>("SearchImageGoogle");
    module.register_handler<web::UrbanDictionary>("UrbanDictionary");
    module.register_handler<web::SearchWebSearx>("SearchWebSearx");
    module.register_handler<web::VideoInfo>("VideoInfo");
    module.register_handler<web::MediaWiki>("MediaWiki");
    module.register_handler<web::MediaWikiTitles>("MediaWikiTitles");

    return module;
}
