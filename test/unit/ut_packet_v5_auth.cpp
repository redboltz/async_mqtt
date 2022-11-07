// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "../common/test_main.hpp"
#include "../common/global_fixture.hpp"

#include <async_mqtt/packet/v5_auth.hpp>
#include <async_mqtt/packet/packet_iterator.hpp>

BOOST_AUTO_TEST_SUITE(ut_packet)

namespace am = async_mqtt;

BOOST_AUTO_TEST_CASE(v5_auth) {
    auto props = am::properties{
        am::property::reason_string("some reason")
    };
    auto p = am::v5::auth_packet{
        am::auth_reason_code::continue_authentication,
        props
    };
    BOOST_TEST(p.code() == am::auth_reason_code::continue_authentication);
    BOOST_TEST(p.props() == props);
    {
        auto cbs = p.const_buffer_sequence();
        char expected[] {
            char(0xf0),                         // fixed_header
            0x10,                               // remaining_length
            0x18,                               // reason_code
            0x0e,                               // property_length
            0x1f,                               // reason_string
            0x00, 0x0b, 0x73, 0x6f, 0x6d, 0x65, 0x20, 0x72, 0x65, 0x61, 0x73, 0x6f, 0x6e,
        };
        auto [b, e] = am::make_packet_range(cbs);
        BOOST_TEST(std::equal(b, e, std::begin(expected)));

        auto buf = am::allocate_buffer(std::begin(expected), std::end(expected));
        auto p = am::v5::auth_packet(buf);
        BOOST_TEST(p.code() == am::auth_reason_code::continue_authentication);
        BOOST_TEST(p.props() == props);

        auto cbs2 = p.const_buffer_sequence();
        auto [b2, e2] = am::make_packet_range(cbs2);
        BOOST_TEST(std::equal(b2, e2, std::begin(expected)));
    }
}

BOOST_AUTO_TEST_CASE(v5_auth_no_arg) {
    auto p = am::v5::auth_packet{};
    BOOST_TEST(p.code() == am::auth_reason_code::success);
    BOOST_TEST(p.props().empty());

    {
        auto cbs = p.const_buffer_sequence();
        char expected[] {
            char(0xf0),                         // fixed_header
            0x00,                               // remaining_length
        };
        auto [b, e] = am::make_packet_range(cbs);
        BOOST_TEST(std::equal(b, e, std::begin(expected)));

        auto buf = am::allocate_buffer(std::begin(expected), std::end(expected));
        auto p = am::v5::auth_packet(buf);
        BOOST_TEST(p.code() == am::auth_reason_code::success);
        BOOST_TEST(p.props().empty());

        auto cbs2 = p.const_buffer_sequence();
        auto [b2, e2] = am::make_packet_range(cbs2);
        BOOST_TEST(std::equal(b2, e2, std::begin(expected)));
    }
}

BOOST_AUTO_TEST_CASE(v5_auth_pid_rc) {
    auto p = am::v5::auth_packet{
        am::auth_reason_code::success
    };
    BOOST_TEST(p.code() == am::auth_reason_code::success);
    BOOST_TEST(p.props().empty());

    {
        auto cbs = p.const_buffer_sequence();
        char expected[] {
            char(0xf0),                         // fixed_header
            0x01,                               // remaining_length
            0x00,                               // reason_code
        };
        auto [b, e] = am::make_packet_range(cbs);
        BOOST_TEST(std::equal(b, e, std::begin(expected)));

        auto buf = am::allocate_buffer(std::begin(expected), std::end(expected));
        auto p = am::v5::auth_packet(buf);
        BOOST_TEST(p.code() == am::auth_reason_code::success);
        BOOST_TEST(p.props().empty());

        auto cbs2 = p.const_buffer_sequence();
        auto [b2, e2] = am::make_packet_range(cbs2);
        BOOST_TEST(std::equal(b2, e2, std::begin(expected)));
    }
}

BOOST_AUTO_TEST_CASE(v5_auth_prop_len_last) {
    char expected[] {
        char(0xf0),                         // fixed_header
        0x02,                               // remaining_length
        0x00,                               // reason_code
        0x00,                               // property_length
    };
    auto buf = am::allocate_buffer(std::begin(expected), std::end(expected));
    auto p = am::v5::auth_packet(buf);
    BOOST_TEST(p.code() == am::auth_reason_code::success);
    BOOST_TEST(p.props().empty());

    auto cbs = p.const_buffer_sequence();
    auto [b, e] = am::make_packet_range(cbs);
    BOOST_TEST(std::equal(b, e, std::begin(expected)));
}

BOOST_AUTO_TEST_SUITE_END()
