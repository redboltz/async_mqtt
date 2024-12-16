// Copyright udonmo 2018
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "../common/test_main.hpp"
#include "../common/global_fixture.hpp"

#include <async_mqtt/util/utf8validate.hpp>


BOOST_AUTO_TEST_SUITE(ut_utf8_validate)

namespace am = async_mqtt;
using namespace std::string_literals;
using namespace std::string_view_literals;

BOOST_AUTO_TEST_CASE( one_byte ) {
    // nul charactor
    BOOST_TEST(!am::utf8string_check("\x0"sv));

    // control charactor
    BOOST_TEST(!am::utf8string_check("\x1"sv));

    // control charactor
    BOOST_TEST(!am::utf8string_check("\x1f"sv));

    // valid charactor(0x20)
    BOOST_TEST(am::utf8string_check(" "sv));

    // valid charactor(0x7e)
    BOOST_TEST(am::utf8string_check("~"sv));

    // control charactor
    BOOST_TEST(!am::utf8string_check("\x7f"sv));
}

BOOST_AUTO_TEST_CASE( two_bytes ) {
    // valid encoded string case 110XXXXx 10xxxxxx
    // included invalid encoded utf8
    // case 110XXXXx 11xxxxxx
    //                ^
    BOOST_TEST(
        !am::utf8string_check(
            std::string{static_cast<char>(0b1100'0010u), static_cast<char>(0b1100'0000u)}
        )
    );

    // included invalid encoded utf8
    // case 110XXXXx 00xxxxxx
    //               ^
    BOOST_TEST(
        !am::utf8string_check(
            std::string{static_cast<char>(0b1100'0010u), static_cast<char>(0b0000'0000u)}
        )
    );

    // included invalid encoded utf8
    // case 111XXXXx 10xxxxxx
    //        ^
    BOOST_TEST(
        !am::utf8string_check(
            std::string{static_cast<char>(0b1110'0010u), static_cast<char>(0b1000'0000u)}
        )
    );

    // included invalid encoded utf8
    // case 100XXXXx 10xxxxxx
    //       ^
    BOOST_TEST(
        !am::utf8string_check(
            std::string{static_cast<char>(0b1000'0010u), static_cast<char>(0b1000'0000u)}
        )
    );

    // included invalid encoded utf8
    // case 010XXXXx 10xxxxxx
    //      ^
    BOOST_TEST(
        !am::utf8string_check(
            std::string{static_cast<char>(0b0100'0010u), static_cast<char>(0b1000'0000u)}
        )
    );

    // overlong utf8
    // case U+0000
    BOOST_TEST(
        !am::utf8string_check(
            std::string{static_cast<char>(0b1100'0000u), static_cast<char>(0b1000'0000u)}
        )
    );

    // overlong utf8
    // case U+007F
    BOOST_TEST(
        !am::utf8string_check(
            std::string{static_cast<char>(0b1100'0001u), static_cast<char>(0b1011'1111u)}
        )
    );
}

BOOST_AUTO_TEST_CASE( three_bytes ) {

    // three bytes charactor

    // valid encoded string case 1110XXXX 10Xxxxxx 10xxxxxx
    // included invalid encoded utf8
    // case 1110XXXX 10Xxxxxx 11xxxxxx
    //                         ^
    BOOST_TEST(
        !am::utf8string_check(
            std::string{
                static_cast<char>(0b1110'0000u),
                static_cast<char>(0b1010'0000u),
                static_cast<char>(0b1100'0000u)
            }
        )
    );

    // included invalid encoded utf8
    // case 1110XXXX 10Xxxxxx 00xxxxxx
    //                        ^
    BOOST_TEST(
        !am::utf8string_check(
            std::string{
                static_cast<char>(0b1110'0000u),
                static_cast<char>(0b1010'0000u),
                static_cast<char>(0b0000'0000u)
            }
        )
    );

    // included invalid encoded utf8
    // case 1110XXXX 11Xxxxxx 10xxxxxx
    //                ^
    BOOST_TEST(
        !am::utf8string_check(
            std::string{
                static_cast<char>(0b1110'0000u),
                static_cast<char>(0b1110'0000u),
                static_cast<char>(0b1000'0000u)
            }
        )
    );

    // included invalid encoded utf8
    // case 1110XXXX 00Xxxxxx 10xxxxxx
    //               ^
    BOOST_TEST(
        !am::utf8string_check(
            std::string{
                static_cast<char>(0b1110'0000u),
                static_cast<char>(0b0010'0000u),
                static_cast<char>(0b1000'0000u)
            }
        )
    );

    // included invalid encoded utf8
    // case 1111XXXX 10Xxxxxx 10xxxxxx
    //         ^
    BOOST_TEST(
        !am::utf8string_check(
            std::string{
                static_cast<char>(0b1111'0000u),
                static_cast<char>(0b1010'0000u),
                static_cast<char>(0b1000'0000u)
            }
        )
    );

    // included invalid encoded utf8
    // case 1100XXXX 10Xxxxxx 10xxxxxx
    //        ^
    BOOST_TEST(
        !am::utf8string_check(
            std::string{
                static_cast<char>(0b1100'0000u),
                static_cast<char>(0b1010'0000u),
                static_cast<char>(0b1000'0000u)
            }
        )
    );

    // included invalid encoded utf8
    // case 1010XXXX 10Xxxxxx 10xxxxxx
    //       ^
    BOOST_TEST(
        !am::utf8string_check(
            std::string{
                static_cast<char>(0b1010'0000u),
                static_cast<char>(0b1010'0000u),
                static_cast<char>(0b1000'0000u)
            }
        )
    );

    // included invalid encoded utf8
    // case 0110XXXX 10Xxxxxx 10xxxxxx
    //      ^
    BOOST_TEST(
        !am::utf8string_check(
            std::string{
                static_cast<char>(0b0110'0000u),
                static_cast<char>(0b1010'0000u),
                static_cast<char>(0b1000'0000u)
            }
        )
    );


    // included overlong utf8
    // case U+0000
    BOOST_TEST(
        !am::utf8string_check(
            std::string{
                static_cast<char>(0b1110'0000u),
                static_cast<char>(0b1000'0000u),
                static_cast<char>(0b1000'0000u)
            }
        )
    );

    // included overlong utf8
    // case U+07FF
    BOOST_TEST(
        !am::utf8string_check(
            std::string{
                static_cast<char>(0b1110'0000u),
                static_cast<char>(0b1001'1111u),
                static_cast<char>(0b1011'1111u)
            }
        )
    );


    // included surrogate utf8
    // case U+D800
    BOOST_TEST(
        !am::utf8string_check(
            std::string{
                static_cast<char>(0b1110'1101u),
                static_cast<char>(0b1010'0000u),
                static_cast<char>(0b1000'0000u)
            }
        )
    );

    // included surrogate utf8
    // case U+DFFF
    BOOST_TEST(
        !am::utf8string_check(
            std::string{
                static_cast<char>(0b1110'1101u),
                static_cast<char>(0b1011'1111u),
                static_cast<char>(0b1011'1111u)
            }
        )
    );
}

BOOST_AUTO_TEST_CASE( four_bytes ) {
    // valid encoded string case 11110XXX 10XXxxxx 10xxxxxx 10xxxxxx
    // included invalid encoded utf8
    // case 11110XXX 10XXxxxx 10xxxxxx 11xxxxxx
    //                                  ^
    BOOST_TEST(
        !am::utf8string_check(
            std::string{
                static_cast<char>(0b1111'0000u),
                static_cast<char>(0b1001'0000u),
                static_cast<char>(0b1000'0000u),
                static_cast<char>(0b1100'0000u)
            }
        )
    );

    // included invalid encoded utf8
    // case 11110XXX 10XXxxxx 10xxxxxx 00xxxxxx
    //                                 ^
    BOOST_TEST(
        !am::utf8string_check(
            std::string{
                static_cast<char>(0b1111'0000u),
                static_cast<char>(0b1001'0000u),
                static_cast<char>(0b1000'0000u),
                static_cast<char>(0b0000'0000u)
            }
        )
    );

    // included invalid encoded utf8
    // case 11110XXX 10XXxxxx 11xxxxxx 10xxxxxx
    //                         ^
    BOOST_TEST(
        !am::utf8string_check(
            std::string{
                static_cast<char>(0b1111'0000u),
                static_cast<char>(0b1001'0000u),
                static_cast<char>(0b1100'0000u),
                static_cast<char>(0b1000'0000u)
            }
        )
    );

    // included invalid encoded utf8
    // case 11110XXX 10XXxxxx 00xxxxxx 10xxxxxx
    //                        ^
    BOOST_TEST(
        !am::utf8string_check(
            std::string{
                static_cast<char>(0b1111'0000u),
                static_cast<char>(0b1001'0000u),
                static_cast<char>(0b0000'0000u),
                static_cast<char>(0b1000'0000u)
            }
        )
    );

    // included invalid encoded utf8
    // case 11110XXX 11XXxxxx 10xxxxxx 10xxxxxx
    //                ^
    BOOST_TEST(
        !am::utf8string_check(
            std::string{
                static_cast<char>(0b1111'0000u),
                static_cast<char>(0b1101'0000u),
                static_cast<char>(0b1000'0000u),
                static_cast<char>(0b1000'0000u)
            }
        )
    );

    // included invalid encoded utf8
    // case 11110XXX 00XXxxxx 10xxxxxx 10xxxxxx
    //               ^
    BOOST_TEST(
        !am::utf8string_check(
            std::string{
                static_cast<char>(0b1111'0000u),
                static_cast<char>(0b0001'0000u),
                static_cast<char>(0b1000'0000u),
                static_cast<char>(0b1000'0000u)
            }
        )
    );

    // included invalid encoded utf8
    // case 11111XXX 10XXxxxx 10xxxxxx 10xxxxxx
    //          ^
    BOOST_TEST(
        !am::utf8string_check(
            std::string{
                static_cast<char>(0b1111'1000u),
                static_cast<char>(0b1001'0000u),
                static_cast<char>(0b1000'0000u),
                static_cast<char>(0b1000'0000u)
            }
        )
    );

    // included invalid encoded utf8
    // case 11100XXX 10XXxxxx 10xxxxxx 10xxxxxx
    //         ^
    BOOST_TEST(
        !am::utf8string_check(
            std::string{
                static_cast<char>(0b1110'0000u),
                static_cast<char>(0b1001'0000u),
                static_cast<char>(0b1000'0000u),
                static_cast<char>(0b1000'0000u)
            }
        )
    );

    // included invalid encoded utf8
    // case 11010XXX 10XXxxxx 10xxxxxx 10xxxxxx
    //        ^
    BOOST_TEST(
        !am::utf8string_check(
            std::string{
                static_cast<char>(0b1101'0000u),
                static_cast<char>(0b1001'0000u),
                static_cast<char>(0b1000'0000u),
                static_cast<char>(0b1000'0000u)
            }
        )
    );

    // included invalid encoded utf8
    // case 10110XXX 10XXxxxx 10xxxxxx 10xxxxxx
    //       ^
    BOOST_TEST(
        !am::utf8string_check(
            std::string{
                static_cast<char>(0b1011'0000u),
                static_cast<char>(0b1001'0000u),
                static_cast<char>(0b1000'0000u),
                static_cast<char>(0b1000'0000u)
            }
        )
    );

    // included invalid encoded utf8
    // case 01110XXX 10XXxxxx 10xxxxxx 10xxxxxx
    //      ^
    BOOST_TEST(
        !am::utf8string_check(
            std::string{
                static_cast<char>(0b0111'0000u),
                static_cast<char>(0b1001'0000u),
                static_cast<char>(0b1000'0000u),
                static_cast<char>(0b1000'0000u)
            }
        )
    );

    // included overlong utf8
    // case U+0000
    BOOST_TEST(
        !am::utf8string_check(
            std::string{
                static_cast<char>(0b1111'0000u),
                static_cast<char>(0b1000'0000u),
                static_cast<char>(0b1000'0000u),
                static_cast<char>(0b1000'0000u)
            }
        )
    );

    // included overlong utf8
    // case U+FFFF
    BOOST_TEST(
        !am::utf8string_check(
            std::string{
                static_cast<char>(0b1111'0000u),
                static_cast<char>(0b1000'1111u),
                static_cast<char>(0b1011'1111u),
                static_cast<char>(0b1011'1111u)
            }
        )
    );
}

BOOST_AUTO_TEST_CASE( combination ) {
    // included invalid charactor
    BOOST_TEST(
        !am::utf8string_check(
            std::string{
                'a', '\x01', '\x00'
            }
        )
    );

    // included non charactor
    BOOST_TEST(
        !am::utf8string_check(
            std::string{
                'a', '\x01'
            }
        )
    );
}
BOOST_AUTO_TEST_SUITE_END()
