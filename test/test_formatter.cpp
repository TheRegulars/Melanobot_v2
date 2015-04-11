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
#define BOOST_TEST_MODULE Test_Formatter

#include <boost/test/unit_test.hpp>

#include "string/string.hpp"
#include <string/logger.hpp>
#include "xonotic/formatter.hpp"
#include "irc/irc_formatter.hpp"

using namespace string;

BOOST_AUTO_TEST_CASE( test_registry )
{
    Logger::instance().set_log_verbosity("sys",-1);
    Formatter::registry();
    BOOST_CHECK( Formatter::formatter("config")->name() == "config" );
    BOOST_CHECK( Formatter::formatter("utf8")->name() == "utf8" );
    BOOST_CHECK( Formatter::formatter("ascii")->name() == "ascii" );
    BOOST_CHECK( Formatter::formatter("ansi-ascii")->name() == "ansi-ascii" );
    BOOST_CHECK( Formatter::formatter("ansi-utf8")->name() == "ansi-utf8" );
    BOOST_CHECK( Formatter::formatter("foobar")->name() == "utf8" );
}

template<class T>
    const T* cast(FormattedString::const_reference item)
{
    return dynamic_cast<const T*>(item.get());
}

BOOST_AUTO_TEST_CASE( test_utf8 )
{
    FormatterUtf8 fmt;
    std::string utf8 = u8"Foo bar è$ç";
    auto decoded = fmt.decode(utf8);
    BOOST_CHECK( decoded.size() == 4 );
    BOOST_CHECK( cast<AsciiSubstring>(decoded[0]));
    BOOST_CHECK( cast<Unicode>(decoded[1]));
    BOOST_CHECK( cast<AsciiSubstring>(decoded[2]));
    BOOST_CHECK( cast<Unicode>(decoded[3]));
    BOOST_CHECK( decoded.encode(fmt) == utf8 );
    auto invalid_utf8 = utf8;
    invalid_utf8.push_back(uint8_t(0b1110'0000)); // '));
    BOOST_CHECK( fmt.decode(invalid_utf8).encode(fmt) == utf8 );
}

BOOST_AUTO_TEST_CASE( test_ascii )
{
    string::FormatterAscii fmt;
    std::string utf8 = u8"Foo bar è$ç";
    BOOST_CHECK( string::FormatterUtf8().decode(utf8).encode(fmt) == "Foo bar ?$?" );
}

BOOST_AUTO_TEST_CASE( test_config )
{
    FormatterConfig fmt;
    std::string utf8 = u8"Foo bar è$ç";
    auto decoded = fmt.decode(utf8);
    BOOST_CHECK( decoded.size() == 4 );
    BOOST_CHECK( cast<AsciiSubstring>(decoded[0]));
    BOOST_CHECK( cast<Unicode>(decoded[1]));
    BOOST_CHECK( cast<AsciiSubstring>(decoded[2]));
    BOOST_CHECK( cast<Unicode>(decoded[3]));
    BOOST_CHECK( decoded.encode(fmt) == utf8 );

    std::string formatted = "Hello #1#World #-bu#test#-###1#green#green#x00f#blue§";
    decoded = fmt.decode(formatted);
    BOOST_CHECK( decoded.size() == 12 );
    // "Hello "
    BOOST_CHECK( cast<AsciiSubstring>(decoded[0]) );
    BOOST_CHECK( cast<AsciiSubstring>(decoded[0])->string() == "Hello " );
    // red
    BOOST_CHECK( cast<Color>(decoded[1]) );
    BOOST_CHECK( cast<Color>(decoded[1])->color() == color::red );
    // "World "
    BOOST_CHECK( cast<AsciiSubstring>(decoded[2]) );
    BOOST_CHECK( cast<AsciiSubstring>(decoded[2])->string() == "World " );
    // bold+underline
    BOOST_CHECK( cast<Format>(decoded[3]) );
    BOOST_CHECK( cast<Format>(decoded[3])->flags() == (FormatFlags::BOLD|FormatFlags::UNDERLINE) );
    // "test"
    BOOST_CHECK( cast<AsciiSubstring>(decoded[4]) );
    BOOST_CHECK( cast<AsciiSubstring>(decoded[4])->string() == "test" );
    // clear
    BOOST_CHECK( cast<ClearFormatting>(decoded[5]) );
    // "#1"
    BOOST_CHECK( cast<AsciiSubstring>(decoded[6]) );
    BOOST_CHECK( cast<AsciiSubstring>(decoded[6])->string() == "#1" );
    // green
    BOOST_CHECK( cast<Color>(decoded[7]) );
    BOOST_CHECK( cast<Color>(decoded[7])->color() == color::green );
    // "green"
    BOOST_CHECK( cast<AsciiSubstring>(decoded[8]) );
    BOOST_CHECK( cast<AsciiSubstring>(decoded[8])->string() == "green" );
    // blue
    BOOST_CHECK( cast<Color>(decoded[9]) );
    BOOST_CHECK( cast<Color>(decoded[9])->color() == color::blue );
    // "blue"
    BOOST_CHECK( cast<AsciiSubstring>(decoded[10]) );
    BOOST_CHECK( cast<AsciiSubstring>(decoded[10])->string() == "blue" );
    // Unicode §
    BOOST_CHECK( cast<Unicode>(decoded[11]) );
    BOOST_CHECK( cast<Unicode>(decoded[11])->utf8() == u8"§" );
    BOOST_CHECK( cast<Unicode>(decoded[11])->point() == 0x00A7 );

    BOOST_CHECK( decoded.encode(fmt) == "Hello #1#World #-bu#test#-###1#2#green#4#blue§" );
}
