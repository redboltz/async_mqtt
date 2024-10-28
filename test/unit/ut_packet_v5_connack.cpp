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
struct v5_connack;
struct v5_connack_error;
BOOST_AUTO_TEST_SUITE_END()

#include <async_mqtt/protocol/packet/v5_connack.hpp>
#include <async_mqtt/protocol/packet/packet_iterator.hpp>
#include <async_mqtt/protocol/packet/packet_traits.hpp>

BOOST_AUTO_TEST_SUITE(ut_packet)

namespace am = async_mqtt;
using namespace std::literals::string_view_literals;

BOOST_AUTO_TEST_CASE(v5_connack) {
    BOOST_TEST(am::is_connack<am::v5::connack_packet>());
    BOOST_TEST(!am::is_v3_1_1<am::v5::connack_packet>());
    BOOST_TEST(am::is_v5<am::v5::connack_packet>());
    BOOST_TEST(!am::is_client_sendable<am::v5::connack_packet>());
    BOOST_TEST(am::is_server_sendable<am::v5::connack_packet>());

    auto props = am::properties{
        am::property::session_expiry_interval(0x0fffffff)
    };
    auto p = am::v5::connack_packet{
        true,   // session_present
        am::connect_reason_code::not_authorized,
        props
    };
    BOOST_TEST(p.session_present());
    BOOST_TEST(p.code() == am::connect_reason_code::not_authorized);
    BOOST_TEST(p.props() == props);

    {
        auto cbs = p.const_buffer_sequence();
        BOOST_TEST(cbs.size() == p.num_of_const_buffer_sequence());
        char expected[] {
            0x20,       // fixed_header
            0x08,       // remaining_length
            0x01,       // session_present
            char(0x87), // connect_reason_code
            0x05,       // property_length
            0x11, 0x0f, char(0xff), char(0xff), char(0xff), // session_expiry_interval
        };
        auto [b, e] = am::make_packet_range(cbs);
        BOOST_TEST(std::equal(b, e, std::begin(expected)));

        am::buffer buf{std::begin(expected), std::end(expected)};
        am::error_code ec;
        auto p = am::v5::connack_packet{buf, ec};
        BOOST_TEST(!ec);
        BOOST_TEST(p.session_present());
        BOOST_TEST(p.code() == am::connect_reason_code::not_authorized);
        BOOST_TEST(p.props() == props);

        auto cbs2 = p.const_buffer_sequence();
        BOOST_TEST(cbs2.size() == p.num_of_const_buffer_sequence());
        auto [b2, e2] = am::make_packet_range(cbs2);
        BOOST_TEST(std::equal(b2, e2, std::begin(expected)));

        BOOST_TEST(p.type() == am::control_packet_type::connack);
    }
    BOOST_TEST(
        boost::lexical_cast<std::string>(p) ==
        "v5::connack{rc:not_authorized,sp:1,ps:[{id:session_expiry_interval,val:268435455}]}"
    );

    auto p2 = am::v5::connack_packet{
        true,   // session_present
        am::connect_reason_code::not_authorized,
        props
    };
    auto p3 = am::v5::connack_packet{
        true,   // session_present
        am::connect_reason_code::success,
        props
    };
    BOOST_CHECK(p == p2);
    BOOST_CHECK(!(p < p2));
    BOOST_CHECK(!(p2< p));
    BOOST_CHECK(p < p3 || p3 < p);

    try {
        auto p = am::v5::connack_packet{
            true,   // session_present
            am::connect_reason_code::success,
            am::properties{
                am::property::will_delay_interval{1}
            }
        };
        BOOST_TEST(false);
    }
    catch (am::system_error const& se) {
        BOOST_TEST(se.code() == am::disconnect_reason_code::malformed_packet);
    }
}

BOOST_AUTO_TEST_CASE(v5_connack_error) {
    {
        am::buffer buf; // empty
        am::error_code ec;
        am::v5::connack_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        am::buffer buf{"\x00"sv}; // invalid type
        am::error_code ec;
        am::v5::connack_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        am::buffer buf{"\x20"sv}; // short
        am::error_code ec;
        am::v5::connack_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        am::buffer buf{"\x20\x01"sv}; // invalid reserved
        am::error_code ec;
        am::v5::connack_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        am::buffer buf{"\x20\x00"sv}; // remaining length match short
        am::error_code ec;
        am::v5::connack_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        am::buffer buf{"\x20\x02"sv}; // short
        am::error_code ec;
        am::v5::connack_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        am::buffer buf{"\x20\x02\x02\x00"sv}; // invalid reserved flag
        am::error_code ec;
        am::v5::connack_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        am::buffer buf{"\x20\x02\x00\xff"sv}; // invalid reserved code
        am::error_code ec;
        am::v5::connack_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        am::buffer buf{"\x20\x03\x00\x00\x00"sv}; // zero property length
        am::error_code ec;
        am::v5::connack_packet{buf, ec};
        BOOST_TEST(ec == am::error_code{});
    }
    {
        am::buffer buf{"\x20\x06\x00\x00\xff\xff\xff\x80"sv}; // over property length
        am::error_code ec;
        am::v5::connack_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        am::buffer buf{"\x20\x03\x00\x00\x01"sv}; // short
        am::error_code ec;
        am::v5::connack_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        am::buffer buf{"\x20\x04\x00\x00\x00\x00"sv}; // long
        am::error_code ec;
        am::v5::connack_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
}

BOOST_AUTO_TEST_SUITE_END()
