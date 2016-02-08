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
#ifndef MELANOBOT_STRING_REPLACEMENTS_HPP
#define MELANOBOT_STRING_REPLACEMENTS_HPP
#include "string.hpp"

namespace string {

/**
 * \brief Element that is used for replacements
 */
class Placeholder : public Element
{
public:
    Placeholder(std::string identifier, FormattedString replacement = {})
        : identifier(std::move(identifier)),
          replacement(std::move(replacement))
    {}

    std::string to_string(const Formatter& formatter) const override
    {
        return replacement.encode(formatter);
    }

    int char_count() const override
    {
        return replacement.char_count();
    }

    std::unique_ptr<Element> clone() const override
    {
        return melanolib::New<Placeholder>(identifier, replacement.copy());
    }

    void replace(const ReplacementFunctor& func) override
    {
        auto rep = func(identifier);
        if ( rep )
            replacement = std::move(*rep);
    }

private:
    std::string identifier;
    FormattedString replacement;
};


class FilterRegistry : public melanolib::Singleton<FilterRegistry>
{
public:
    using Filter = std::function<FormattedString (const std::vector<FormattedString>& arguments)>;

    void register_filter(const std::string& name, const Filter& filter)
    {
        filters[name] = filter;
    }

    void unregister_filter(const std::string& name)
    {
        filters.erase(name);
    }

    FormattedString apply_filter(
        const std::string& name,
        const std::vector<FormattedString>& arguments
    ) const
    {
        auto it = filters.find(name);
        if ( it == filters.end() )
        {
            if ( arguments.empty() )
                return {};
            return arguments[0];
        }
        return it->second(arguments);
    }

private:
    FilterRegistry(){}
    friend ParentSingleton;

    std::unordered_map<std::string, Filter> filters;
};

class FilterCall : public Element
{
public:
    FilterCall(std::string filter, std::vector<FormattedString> arguments)
        : filter(std::move(filter)),
          arguments(std::move(arguments))
    {}

    std::string to_string(const Formatter& formatter) const override
    {
        return filtered().encode(formatter);
    }

    int char_count() const override
    {
        return filtered().char_count();
    }

    std::unique_ptr<Element> clone() const override
    {
        return melanolib::New<FilterCall>(filter, arguments);
    }

    void replace(const ReplacementFunctor& func) override
    {
        for ( auto& arg : arguments )
            arg.replace(func);
    }

    FormattedString filtered() const
    {
        return FilterRegistry::instance().apply_filter(filter, arguments);
    }

private:
    std::string filter;
    std::vector<FormattedString> arguments;
};

} // namespace string
#endif // MELANOBOT_STRING_REPLACEMENTS_HPP