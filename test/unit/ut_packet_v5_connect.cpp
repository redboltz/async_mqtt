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
struct v5_connect;
struct v5_connect_error;
BOOST_AUTO_TEST_SUITE_END()

#include <async_mqtt/packet/v5_connect.hpp>
#include <async_mqtt/packet/packet_iterator.hpp>
#include <async_mqtt/packet/packet_traits.hpp>

BOOST_AUTO_TEST_SUITE(ut_packet)

namespace am = async_mqtt;
using namespace std::literals::string_view_literals;

BOOST_AUTO_TEST_CASE(v5_connect) {
    BOOST_TEST(am::is_connect<am::v5::connect_packet>());
    BOOST_TEST(!am::is_v3_1_1<am::v5::connect_packet>());
    BOOST_TEST(am::is_v5<am::v5::connect_packet>());
    BOOST_TEST(am::is_client_sendable<am::v5::connect_packet>());
    BOOST_TEST(!am::is_server_sendable<am::v5::connect_packet>());

    auto w = am::will{
        "topic1",
        "payload1",
        am::pub::retain::yes | am::qos::at_least_once,
        am::properties{
            am::property::will_delay_interval(0x0fffffff),
            am::property::content_type("json")
        }
    };

    auto props = am::properties{
        am::property::session_expiry_interval(0x0fffffff),
        am::property::user_property("mykey", "myval")
    };

    auto p = am::v5::connect_packet{
        true,   // clean_start
        0x1234, // keep_alive
        "cid1",
        w,
        "user1",
        "pass1",
        props
    };

    BOOST_TEST(p.clean_start());
    BOOST_TEST(p.keep_alive() == 0x1234);
    BOOST_TEST(p.client_id() == "cid1");
    BOOST_TEST(p.get_will().has_value());
    BOOST_TEST(*p.get_will() == w);
    BOOST_TEST(p.user_name().has_value());
    BOOST_TEST(*p.user_name() == "user1");
    BOOST_TEST(p.password().has_value());
    BOOST_TEST(*p.password() == "pass1");
    BOOST_TEST(p.props() == props);
    {
        auto cbs = p.const_buffer_sequence();
        BOOST_TEST(cbs.size() == p.num_of_const_buffer_sequence());
        char expected[] {
            0x10,                               // fixed_header
            0x52,                               // remaining_length
            0x00, 0x04, 'M', 'Q', 'T', 'T',     // protocol_name
            0x05,                               // protocol_version
            char(0xee),                         // connect_flags
            0x12, 0x34,                         // keep_alive
            0x14,                               // property_length
            0x11, 0x0f, char(0xff), char(0xff), char(0xff), // session_expiry_interval
            0x26,                               // user_property
            0x00, 0x05, 0x6d, 0x79, 0x6b, 0x65, 0x79, // mykey
            0x00, 0x05, 0x6d, 0x79, 0x76, 0x61, 0x6c, // myval
            0x00, 0x04,                         // client_id_length
            0x63, 0x69, 0x64, 0x31,             // client_id
            0x0c,                               // will_property_length
            0x18, 0x0f, char(0xff), char(0xff), char(0xff), // will_delay_interval
            0x03,                               // content_type
            0x00, 0x04, 0x6a, 0x73, 0x6f, 0x6e, // json
            0x00, 0x06,                         // will_topic_name_length
            0x74, 0x6f, 0x70, 0x69, 0x63, 0x31, // will_topic_name
            0x00, 0x08,                         // will_message_length
            0x70, 0x61, 0x79, 0x6c, 0x6f, 0x61, 0x64, 0x31, // will_message
            0x00, 0x05,                         // user_name_length
            0x75, 0x73, 0x65, 0x72, 0x31,       // user_name
            0x00, 0x05,                         // password_length
            0x70, 0x61, 0x73, 0x73, 0x31,       // password
        };
        auto [b, e] = am::make_packet_range(cbs);
        BOOST_TEST(std::equal(b, e, std::begin(expected)));

        am::buffer buf{std::begin(expected), std::end(expected)};
        am::error_code ec;
        auto p = am::v5::connect_packet{buf, ec};
        BOOST_TEST(!ec);
        BOOST_TEST(p.clean_start());
        BOOST_TEST(p.keep_alive() == 0x1234);
        BOOST_TEST(p.client_id() == "cid1");
        BOOST_TEST(p.get_will().has_value());
        BOOST_TEST(*p.get_will() == w);
        BOOST_TEST(p.user_name().has_value());
        BOOST_TEST(*p.user_name() == "user1");
        BOOST_TEST(p.password().has_value());
        BOOST_TEST(*p.password() == "pass1");
        BOOST_TEST(p.props() == props);

        auto cbs2 = p.const_buffer_sequence();
        BOOST_TEST(cbs2.size() == p.num_of_const_buffer_sequence());
        auto [b2, e2] = am::make_packet_range(cbs2);
        BOOST_TEST(std::equal(b2, e2, std::begin(expected)));

        BOOST_TEST(p.type() == am::control_packet_type::connect);
    }
    BOOST_TEST(
        boost::lexical_cast<std::string>(p) ==
        "v5::connect{cid:cid1,ka:4660,cs:1,un:user1,pw:*****,will:{topic:topic1,message:payload1,qos:at_least_once,retain:yes,ps:[{id:will_delay_interval,val:268435455},{id:content_type,val:json}]},ps:[{id:session_expiry_interval,val:268435455},{id:user_property,key:mykey,val:myval}]}"
    );

    auto p2 = am::v5::connect_packet{
        true,   // clean_start
        0x1234, // keep_alive
        "cid1",
        w,
        "user1",
        "pass1",
        props
    };
    auto p3 = am::v5::connect_packet{
        false,   // clean_start
        0x1234, // keep_alive
        "cid1",
        w,
        "user1",
        "pass1",
        props
    };
    BOOST_CHECK(p == p2);
    BOOST_CHECK(!(p < p2));
    BOOST_CHECK(!(p2< p));
    BOOST_CHECK(p < p3 || p3 < p);


    try {
        auto p = am::v5::connect_packet{
            true,   // clean_session
            0x1234, // keep_alive
            "\xc2\xc0", // invalid_utf8,
            std::nullopt,
            "user1",
            "pass1"
        };
        BOOST_TEST(false);
    }
    catch (am::system_error const& se) {
        BOOST_TEST(se.code() == am::connect_reason_code::client_identifier_not_valid);
    }

    try {
        auto p = am::v5::connect_packet{
            true,   // clean_session
            0x1234, // keep_alive
            "cid1",
            std::nullopt,
            "\xc2\xc0", // invalid_utf8,
            "pass1"
        };
        BOOST_TEST(false);
    }
    catch (am::system_error const& se) {
        BOOST_TEST(se.code() == am::connect_reason_code::bad_user_name_or_password);
    }

    try {
        auto badw = am::will{
            "\xc2\xc0", // invalid_utf8,
            "payload",
            am::pub::retain::yes | am::qos::at_least_once
        };
        auto p = am::v5::connect_packet{
            true,   // clean_session
            0x1234, // keep_alive
            "cid1",
            badw,
            "user1",
            "pass1"
        };
        BOOST_TEST(false);
    }
    catch (am::system_error const& se) {
        BOOST_TEST(se.code() == am::connect_reason_code::topic_name_invalid);
    }

    try {
        auto badw = am::will{
            "topic1",
            std::string(0x10000, 'w'), // too long
            am::pub::retain::yes | am::qos::at_least_once
        };
        auto p = am::v5::connect_packet{
            true,   // clean_session
            0x1234, // keep_alive
            "cid1",
            badw,
            "user1",
            "pass1"
        };
        BOOST_TEST(false);
    }
    catch (am::system_error const& se) {
        BOOST_TEST(se.code() == am::connect_reason_code::malformed_packet);
    }

    try {
        auto p = am::v5::connect_packet{
            true,   // clean_start
            0x1234, // keep_alive
            "cid1",
            w,
            "user1",
            "pass1",
            am::properties{
                am::property::topic_alias{1} // not allowed
            }
        };
        BOOST_TEST(false);
    }
    catch (am::system_error const& se) {
        BOOST_TEST(se.code() == am::connect_reason_code::malformed_packet);
    }

    try {
        auto badw = am::will{
            "topic1",
            "payload1",
            am::pub::retain::yes | am::qos::at_least_once,
            am::properties{
                am::property::topic_alias{1} // not allowed
            }
        };
        auto p = am::v5::connect_packet{
            true,   // clean_session
            0x1234, // keep_alive
            "cid1",
            badw,
            "user1",
            "pass1"
        };
        BOOST_TEST(false);
    }
    catch (am::system_error const& se) {
        BOOST_TEST(se.code() == am::connect_reason_code::malformed_packet);
    }

}

BOOST_AUTO_TEST_CASE(v5_connect_error) {
    {
        am::buffer buf; // empty
        am::error_code ec;
        am::v5::connect_packet{buf, ec};
        BOOST_TEST(ec == am::connect_reason_code::malformed_packet);
    }
    {
        //                CP
        am::buffer buf{"\x00"sv}; // invalid type
        am::error_code ec;
        am::v5::connect_packet{buf, ec};
        BOOST_TEST(ec == am::connect_reason_code::malformed_packet);
    }
    {
        //                CP
        am::buffer buf{"\x10"sv}; // short
        am::error_code ec;
        am::v5::connect_packet{buf, ec};
        BOOST_TEST(ec == am::connect_reason_code::malformed_packet);
    }
    {
        //                CP  RL
        am::buffer buf{"\x10\x01"sv}; // invalid reserved
        am::error_code ec;
        am::v5::connect_packet{buf, ec};
        BOOST_TEST(ec == am::connect_reason_code::malformed_packet);
    }
    {
        //                CP  RL
        am::buffer buf{"\x10\x01\x00"sv}; // short
        am::error_code ec;
        am::v5::connect_packet{buf, ec};
        BOOST_TEST(ec == am::connect_reason_code::malformed_packet);
    }
    {
        //                CP  RL  FIXED        V
        am::buffer buf{"\x10\x07\x00\x04MQTT\x04"sv}; // invalid version
        am::error_code ec;
        am::v5::connect_packet{buf, ec};
        BOOST_TEST(ec == am::connect_reason_code::unsupported_protocol_version);
    }
    {
        //                CP  RL  FIXED        V
        am::buffer buf{"\x10\x07\x00\x04MQTT\x05"sv}; // short
        am::error_code ec;
        am::v5::connect_packet{buf, ec};
        BOOST_TEST(ec == am::connect_reason_code::malformed_packet);
    }
    {
        //                CP  RL  FIXED        V  CF
        am::buffer buf{"\x10\x08\x00\x04MQTT\x05\x01"sv}; // invalid reserved flags
        am::error_code ec;
        am::v5::connect_packet{buf, ec};
        BOOST_TEST(ec == am::connect_reason_code::malformed_packet);
    }
    {
        //                CP  RL  FIXED        V  CF
        am::buffer buf{"\x10\x08\x00\x04MQTT\x05\x00"sv}; // short
        am::error_code ec;
        am::v5::connect_packet{buf, ec};
        BOOST_TEST(ec == am::connect_reason_code::malformed_packet);
    }
    {
        //                CP  RL  FIXED        V  CF  KALIVE
        am::buffer buf{"\x10\x0a\x00\x04MQTT\x05\x00\x11\x22"sv}; // short
        am::error_code ec;
        am::v5::connect_packet{buf, ec};
        BOOST_TEST(ec == am::connect_reason_code::malformed_packet);
    }
    {
        //                CP  RL  FIXED        V  CF  KALIVE  PL
        am::buffer buf{"\x10\x0b\x00\x04MQTT\x05\x00\x11\x22\x00"sv}; // proplen 0
        am::error_code ec;
        am::v5::connect_packet{buf, ec};
        BOOST_TEST(ec == am::connect_reason_code::malformed_packet);
    }
    {
        //                CP  RL  FIXED        V  CF  KALIVE  PL
        am::buffer buf{"\x10\x0b\x00\x04MQTT\x05\x00\x11\x22\x01"sv}; // proplen short
        am::error_code ec;
        am::v5::connect_packet{buf, ec};
        BOOST_TEST(ec == am::connect_reason_code::malformed_packet);
    }
    {
        //                CP  RL  FIXED        V  CF  KALIVE  PL  CIDL
        am::buffer buf{"\x10\x0d\x00\x04MQTT\x05\x00\x11\x22\x00\x00\x01"sv}; // client id short
        am::error_code ec;
        am::v5::connect_packet{buf, ec};
        BOOST_TEST(ec == am::connect_reason_code::malformed_packet);
    }
    {
        //                CP  RL  FIXED        V  CF  KALIVE  PL  CIDL  CI
        am::buffer buf{"\x10\x0f\x00\x04MQTT\x05\x00\x11\x22\x00\x00\x02\xc2\xc0"sv}; // invalid client identifier
        am::error_code ec;
        am::v5::connect_packet{buf, ec};
        BOOST_TEST(ec == am::connect_reason_code::client_identifier_not_valid);
    }
    {
        //                CP  RL  FIXED        V  CF  KALIVE  PL  CIDL  CI
        am::buffer buf{"\x10\x0f\x00\x04MQTT\x05\x00\x11\x22\x00\x00\x02II"sv}; // valid
        am::error_code ec;
        am::v5::connect_packet{buf, ec};
        BOOST_TEST(ec == am::error_code{});
    }
    {
        //                CP  RL  FIXED        V  CF  KALIVE  PL  CIDL  CI
        am::buffer buf{"\x10\x0f\x00\x04MQTT\x05\x1c\x11\x22\x00\x00\x02II"sv}; // invalid will qos
        am::error_code ec;
        am::v5::connect_packet{buf, ec};
        BOOST_TEST(ec == am::connect_reason_code::malformed_packet);
    }
    {
        //                CP  RL  FIXED        V  CF  KALIVE  PL  CIDL  CI
        am::buffer buf{"\x10\x0f\x00\x04MQTT\x05\x04\x11\x22\x00\x00\x02II"sv}; // no will topic length
        am::error_code ec;
        am::v5::connect_packet{buf, ec};
        BOOST_TEST(ec == am::connect_reason_code::malformed_packet);
    }
    {
        //                CP  RL  FIXED        V  CF  KALIVE  PL  CIDL  CI WPL
        am::buffer buf{"\x10\x10\x00\x04MQTT\x05\x04\x11\x22\x00\x00\x02II\x01"sv}; // will property length short
        am::error_code ec;
        am::v5::connect_packet{buf, ec};
        BOOST_TEST(ec == am::connect_reason_code::malformed_packet);
    }
    {
        //                CP  RL  FIXED        V  CF  KALIVE  PL  CIDL  CI WPL
        am::buffer buf{"\x10\x10\x00\x04MQTT\x05\x04\x11\x22\x00\x00\x02II\x00"sv}; // no will topic length
        am::error_code ec;
        am::v5::connect_packet{buf, ec};
        BOOST_TEST(ec == am::connect_reason_code::malformed_packet);
    }
    {
        //                CP  RL  FIXED        V  CF  KALIVE  PL  CIDL  CI WPL  WTL
        am::buffer buf{"\x10\x12\x00\x04MQTT\x05\x04\x11\x22\x00\x00\x02II\x00\x00\x02"sv}; // no will topic
        am::error_code ec;
        am::v5::connect_packet{buf, ec};
        BOOST_TEST(ec == am::connect_reason_code::malformed_packet);
    }
    {
        //                CP  RL  FIXED        V  CF  KALIVE  PL  CIDL  CI WPL  WTL   WT
        am::buffer buf{"\x10\x14\x00\x04MQTT\x05\x04\x11\x22\x00\x00\x02II\x00\x00\x02\xc2\xc0"sv}; // invalid will topic
        am::error_code ec;
        am::v5::connect_packet{buf, ec};
        BOOST_TEST(ec == am::connect_reason_code::topic_name_invalid);
    }
    {
        //                CP  RL  FIXED        V  CF  KALIVE  PL  CIDL  CI WPL  WTL   WT  WML   WM
        am::buffer buf{"\x10\x14\x00\x04MQTT\x05\x04\x11\x22\x00\x00\x02II\x00\x00\x02WT"sv}; // no will message length
        am::error_code ec;
        am::v5::connect_packet{buf, ec};
        BOOST_TEST(ec == am::connect_reason_code::malformed_packet);
    }
    {
        //                CP  RL  FIXED        V  CF  KALIVE  PL  CIDL  CI WPL  WTL   WT  WML   WM
        am::buffer buf{"\x10\x16\x00\x04MQTT\x05\x04\x11\x22\x00\x00\x02II\x00\x00\x02WT\x00\x01"sv}; // no will message
        am::error_code ec;
        am::v5::connect_packet{buf, ec};
        BOOST_TEST(ec == am::connect_reason_code::malformed_packet);
    }
    {
        //                CP  RL  FIXED        V  CF  KALIVE  PL  CIDL  CI WPL  WTL   WT  WML   WM
        am::buffer buf{"\x10\x18\x00\x04MQTT\x05\x04\x11\x22\x00\x00\x02II\x00\x00\x02WT\x00\x02WM"sv}; // valid
        am::error_code ec;
        am::v5::connect_packet{buf, ec};
        BOOST_TEST(ec == am::error_code{});
    }
    {
        //                CP  RL  FIXED        V  CF  KALIVE  PL  CIDL  CI
        am::buffer buf{"\x10\x0f\x00\x04MQTT\x05\x20\x11\x22\x00\x00\x02II"sv}; // invalid will retain
        am::error_code ec;
        am::v5::connect_packet{buf, ec};
        BOOST_TEST(ec == am::connect_reason_code::malformed_packet);
    }
    {
        //                CP  RL  FIXED        V  CF  KALIVE  PL  CIDL  CI
        am::buffer buf{"\x10\x0f\x00\x04MQTT\x05\x08\x11\x22\x00\x00\x02II"sv}; // invalid will qos
        am::error_code ec;
        am::v5::connect_packet{buf, ec};
        BOOST_TEST(ec == am::connect_reason_code::malformed_packet);
    }
    {
        //                CP  RL  FIXED        V  CF  KALIVE  PL  CIDL  CI  UNL
        am::buffer buf{"\x10\x10\x00\x04MQTT\x05\x80\x11\x22\x00\x00\x02II\x01"sv}; // invalid username length
        am::error_code ec;
        am::v5::connect_packet{buf, ec};
        BOOST_TEST(ec == am::connect_reason_code::malformed_packet);
    }
    {
        //                CP  RL  FIXED        V  CF  KALIVE  PL  CIDL  CI  UNL
        am::buffer buf{"\x10\x11\x00\x04MQTT\x05\x80\x11\x22\x00\x00\x02II\x00\x00"sv}; // zero username length (valid)
        am::error_code ec;
        am::v5::connect_packet{buf, ec};
        BOOST_TEST(ec == am::error_code{});
    }
    {
        //                CP  RL  FIXED        V  CF  KALIVE  PL  CIDL  CI  UNL
        am::buffer buf{"\x10\x11\x00\x04MQTT\x05\x80\x11\x22\x00\x00\x02II\x00\x01"sv}; // short username length
        am::error_code ec;
        am::v5::connect_packet{buf, ec};
        BOOST_TEST(ec == am::connect_reason_code::malformed_packet);
    }
    {
        //                CP  RL  FIXED        V  CF  KALIVE  PL  CIDL  CI  UNL   UN
        am::buffer buf{"\x10\x13\x00\x04MQTT\x05\x80\x11\x22\x00\x00\x02II\x00\x02\xc2\xc0"sv}; // invalid username length
        am::error_code ec;
        am::v5::connect_packet{buf, ec};
        BOOST_TEST(ec == am::connect_reason_code::bad_user_name_or_password);
    }
    {
        //                CP  RL  FIXED        V  CF  KALIVE  PL  CIDL  CI  PWL
        am::buffer buf{"\x10\x10\x00\x04MQTT\x05\x40\x11\x22\x00\x00\x02II\x01"sv}; // invalid password length
        am::error_code ec;
        am::v5::connect_packet{buf, ec};
        BOOST_TEST(ec == am::connect_reason_code::malformed_packet);
    }
    {
        //                CP  RL  FIXED        V  CF  KALIVE  PL  CIDL  CI  PWL
        am::buffer buf{"\x10\x11\x00\x04MQTT\x05\x40\x11\x22\x00\x00\x02II\x00\x00"sv}; // zero password length (valid)
        am::error_code ec;
        am::v5::connect_packet{buf, ec};
        BOOST_TEST(ec == am::error_code{});
    }
    {
        //                CP  RL  FIXED        V  CF  KALIVE  PL  CIDL  CI  PWL
        am::buffer buf{"\x10\x11\x00\x04MQTT\x05\x40\x11\x22\x00\x00\x02II\x00\x01"sv}; // short password length
        am::error_code ec;
        am::v5::connect_packet{buf, ec};
        BOOST_TEST(ec == am::connect_reason_code::malformed_packet);
    }
}

BOOST_AUTO_TEST_SUITE_END()
