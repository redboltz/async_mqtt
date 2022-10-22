// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "../common/test_main.hpp"
#include "../common/global_fixture.hpp"

#include <vector>
#include <async_mqtt/buffer.hpp>

BOOST_AUTO_TEST_SUITE(ut_buffer)

namespace am = async_mqtt;

BOOST_AUTO_TEST_CASE(seq) {
    BOOST_TEST(am::is_buffer_sequence<am::buffer>::value);
    BOOST_TEST(am::is_buffer_sequence<std::vector<am::buffer>>::value);
    BOOST_TEST(am::is_buffer_sequence<std::decay_t<std::vector<am::buffer> const&>>::value);
}

BOOST_AUTO_TEST_CASE( allocate1 ) {
    auto buf = am::allocate_buffer("01234");
    BOOST_TEST(buf == "01234");
    BOOST_TEST(buf.get_life().has_value());
    auto ss1 = buf.substr(2, 3);
    BOOST_TEST(ss1 == "234");
    BOOST_TEST(ss1.get_life().has_value());
    auto ss2 = ss1.substr(1);
    BOOST_TEST(ss2 == "34");
    BOOST_TEST(ss2.get_life().has_value());
}

BOOST_AUTO_TEST_CASE( allocate2 ) {
    std::string s{"01234"};
    auto buf = am::allocate_buffer(s.begin(), s.end());
    BOOST_TEST(buf == "01234");
    BOOST_TEST(buf.get_life().has_value());
    auto ss1 = buf.substr(2, 3);
    BOOST_TEST(ss1 == "234");
    BOOST_TEST(ss1.get_life().has_value());
    auto ss2 = ss1.substr(1);
    BOOST_TEST(ss2 == "34");
    BOOST_TEST(ss2.get_life().has_value());
}

BOOST_AUTO_TEST_CASE( literals ) {
    using namespace am::literals;
    using namespace std::literals::string_view_literals;
    auto buf1 = "abcde"_mb;
    BOOST_TEST(buf1.size() == 5);
    BOOST_TEST(buf1 == "abcde"sv);

    auto buf2 = "ab\0cde"_mb;
    BOOST_TEST(buf2.size() == 6);
    BOOST_TEST(buf2 == "ab\0cde"sv);
}

BOOST_AUTO_TEST_CASE( view ) {
    std::string s{"01234"};
    am::buffer buf{ std::string_view{s} };
    BOOST_TEST(buf == "01234");
    BOOST_TEST(!buf.get_life().has_value());
    auto ss1 = buf.substr(2, 3);
    BOOST_TEST(ss1 == "234");
    BOOST_TEST(!ss1.get_life().has_value());
    auto ss2 = ss1.substr(1);
    BOOST_TEST(ss2 == "34");
    BOOST_TEST(!ss2.get_life().has_value());
}

BOOST_AUTO_TEST_SUITE_END()
