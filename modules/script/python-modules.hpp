/**
 * \file
 * \author Mattia Basaglia
 * \copyright Copyright  Mattia Basaglia
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
#ifndef PYTHON_MODULES_HPP
#define PYTHON_MODULES_HPP

#include <type_traits>

namespace boost {
namespace python {
namespace detail {

/**
 * \brief get_signature for pointer to members, discarding the owner class
 */
template <class Class, class R, class... Args>
    auto get_signature_nomember(R(Class::*)(Args...))
    {
        return boost::mpl::vector<R, Args...>();
    }

template <class Class, class R, class... Args>
    auto get_signature_nomember(R(Class::*)(Args...) const)
    {
        return boost::mpl::vector<R, Args...>();
    }

/**
 * \brief get_signature for functors
 */
template <class T, class = std::enable_if_t<std::is_member_function_pointer<decltype(&T::operator())>::value>>
    auto get_signature(T, void* = 0)
    {
        return get_signature_nomember(decltype(&T::operator())());
    }

}}} // namespace boost::python::detail

#include <boost/python.hpp>
#include "settings.hpp"

namespace python {

/**
 * \brief Namespace corresponding to the python module \c melanobot
 */
namespace melanobot {
using namespace boost::python;


BOOST_PYTHON_MODULE(melanobot)
{
    def("data_file", &settings::data_file);
    def("data_file", [](const std::string& path) { return settings::data_file(path); } );
}

} // namespace melanobot
} // namespace python
#endif // PYTHON_MODULES_HPP
