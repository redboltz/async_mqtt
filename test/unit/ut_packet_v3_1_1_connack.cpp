// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "../common/test_main.hpp"
#include "../common/global_fixture.hpp"

#include <boost/lexical_cast.hpp>

#define ASYNC_MQTT_UNIT_TEST_FOR_PACKET

BOOST_AUTO_TEST_SUITE(ut_packet)
struct v311_connack;
struct v311_connack_error;
BOOST_AUTO_TEST_SUITE_END()

#include <async_mqtt/protocol/packet/v3_1_1_connack.hpp>
#include <async_mqtt/protocol/packet/packet_helper.hpp>
#include <async_mqtt/protocol/packet/packet_traits.hpp>

BOOST_AUTO_TEST_SUITE(ut_packet)

namespace am = async_mqtt;
using namespace std::literals::string_view_literals;

BOOST_AUTO_TEST_CASE(v311_connack) {
    BOOST_TEST(am::is_connack<am::v3_1_1::connack_packet>());
    BOOST_TEST(am::is_v3_1_1<am::v3_1_1::connack_packet>());
    BOOST_TEST(!am::is_v5<am::v3_1_1::connack_packet>());
    BOOST_TEST(!am::is_client_sendable<am::v3_1_1::connack_packet>());
    BOOST_TEST(am::is_server_sendable<am::v3_1_1::connack_packet>());

    auto p = am::v3_1_1::connack_packet{
        true,   // session_present
        am::connect_return_code::not_authorized
    };
    BOOST_TEST(p.session_present());
    BOOST_TEST(p.code() == am::connect_return_code::not_authorized);

    {
        auto cbs = p.const_buffer_sequence();
        BOOST_TEST(cbs.size() == p.num_of_const_buffer_sequence());
        char expected[] {
            0x20, // fixed_header
            0x02, // remaining_length
            0x01, // session_present
            0x05, // connect_return_code
        };
        auto [b, e] = am::make_packet_range(cbs);
        BOOST_TEST(std::equal(b, e, std::begin(expected)));

        am::buffer buf{std::begin(expected), std::end(expected)};
        am::error_code ec;
        auto p = am::v3_1_1::connack_packet{buf, ec};
        BOOST_TEST(!ec);
        BOOST_TEST(p.session_present());
        BOOST_TEST(p.code() == am::connect_return_code::not_authorized);

        auto cbs2 = p.const_buffer_sequence();
        BOOST_TEST(cbs.size() == p.num_of_const_buffer_sequence());
        auto [b2, e2] = am::make_packet_range(cbs2);
        BOOST_TEST(std::equal(b2, e2, std::begin(expected)));

        BOOST_TEST(p.type() == am::control_packet_type::connack);
    }
    BOOST_TEST(
        boost::lexical_cast<std::string>(p) ==
        "v3_1_1::connack{rc:not_authorized,sp:1}"
    );

    auto p2 = am::v3_1_1::connack_packet{
        true,   // session_present
        am::connect_return_code::not_authorized
    };
    auto p3 = am::v3_1_1::connack_packet{
        true,   // session_present
        am::connect_return_code::accepted
    };
    BOOST_CHECK(p == p2);
    BOOST_CHECK(!(p < p2));
    BOOST_CHECK(!(p2< p));
    BOOST_CHECK(p < p3 || p3 < p);
}

BOOST_AUTO_TEST_CASE(v311_connack_error) {
    {
        am::buffer buf; // empty
        am::error_code ec;
        am::v3_1_1::connack_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        am::buffer buf{"\x00"sv}; // invalid type
        am::error_code ec;
        am::v3_1_1::connack_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        am::buffer buf{"\x20"sv}; // short
        am::error_code ec;
        am::v3_1_1::connack_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        am::buffer buf{"\x20\x01"sv}; // invalid remaining length
        am::error_code ec;
        am::v3_1_1::connack_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        am::buffer buf{"\x20\x03"sv}; // invalid remaining length
        am::error_code ec;
        am::v3_1_1::connack_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        am::buffer buf{"\x20\x02"sv}; // short
        am::error_code ec;
        am::v3_1_1::connack_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        am::buffer buf{"\x20\x02\x02\x00"sv}; // invalid reserved flag
        am::error_code ec;
        am::v3_1_1::connack_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        am::buffer buf{"\x20\x02\x00\x06"sv}; // invalid reserved code
        am::error_code ec;
        am::v3_1_1::connack_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
}

BOOST_AUTO_TEST_SUITE_END()
