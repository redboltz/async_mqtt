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
struct v5_unsuback;
struct v5_unsuback_pid4;
struct v5_unsuback_error;
BOOST_AUTO_TEST_SUITE_END()

#include <async_mqtt/protocol/packet/v5_unsuback.hpp>
#include <async_mqtt/protocol/packet/packet_iterator.hpp>
#include <async_mqtt/protocol/packet/packet_traits.hpp>

BOOST_AUTO_TEST_SUITE(ut_packet)

namespace am = async_mqtt;
using namespace std::literals::string_view_literals;

BOOST_AUTO_TEST_CASE(v5_unsuback) {
    BOOST_TEST(am::is_unsuback<am::v5::unsuback_packet>());
    BOOST_TEST(!am::is_v3_1_1<am::v5::unsuback_packet>());
    BOOST_TEST(am::is_v5<am::v5::unsuback_packet>());
    BOOST_TEST(!am::is_client_sendable<am::v5::unsuback_packet>());
    BOOST_TEST(am::is_server_sendable<am::v5::unsuback_packet>());

    std::vector<am::unsuback_reason_code> args {
        am::unsuback_reason_code::no_subscription_existed,
        am::unsuback_reason_code::unspecified_error
    };
    auto props = am::properties{
        am::property::reason_string("some reason")
    };
    auto p = am::v5::unsuback_packet{
        0x1234,         // packet_id
        args,
        props
    };

    BOOST_TEST(p.packet_id() == 0x1234);
    BOOST_TEST((p.entries() == args));
    {
        auto cbs = p.const_buffer_sequence();
        BOOST_TEST(cbs.size() == p.num_of_const_buffer_sequence());
        char expected[] {
            char(0xb0),                                     // fixed_header
            0x13,                                           // remaining_length
            0x12, 0x34,                                     // packet_id
            0x0e,                                           // property_length
            0x1f,                                           // reason_string
            0x00, 0x0b, 0x73, 0x6f, 0x6d, 0x65, 0x20, 0x72, 0x65, 0x61, 0x73, 0x6f, 0x6e,
            0x11,                                           // unsuback_reason_code1
            char(0x80)                                      // unsuback_reason_code2
        };
        auto [b, e] = am::make_packet_range(cbs);
        BOOST_TEST(std::equal(b, e, std::begin(expected)));

        am::buffer buf{std::begin(expected), std::end(expected)};
        am::error_code ec;
        auto p = am::v5::unsuback_packet{buf, ec};
        BOOST_TEST(!ec);
        BOOST_TEST(p.packet_id() == 0x1234);
        BOOST_TEST((p.entries() == args));

        auto cbs2 = p.const_buffer_sequence();
        BOOST_TEST(cbs2.size() == p.num_of_const_buffer_sequence());
        auto [b2, e2] = am::make_packet_range(cbs2);
        BOOST_TEST(std::equal(b2, e2, std::begin(expected)));

        BOOST_TEST(p.type() == am::control_packet_type::unsuback);
    }
    BOOST_TEST(
        boost::lexical_cast<std::string>(p) ==
        "v5::unsuback{pid:4660,[no_subscription_existed,unspecified_error],ps:[{id:reason_string,val:some reason}]}"
    );

    auto p2 = am::v5::unsuback_packet{
        0x1234,         // packet_id
        args,
        props
    };
    auto p3 = am::v5::unsuback_packet{
        0x1235,         // packet_id
        args,
        props
    };
    BOOST_CHECK(p == p2);
    BOOST_CHECK(!(p < p2));
    BOOST_CHECK(!(p2< p));
    BOOST_CHECK(p < p3 || p3 < p);

    try {
        auto p = am::v5::unsuback_packet{
            0x1234,         // packet_id
            args,
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

BOOST_AUTO_TEST_CASE(v5_unsuback_pid4) {
    std::vector<am::unsuback_reason_code> args {
        am::unsuback_reason_code::no_subscription_existed,
        am::unsuback_reason_code::unspecified_error
    };
    auto props = am::properties{
        am::property::reason_string("some reason")
    };
    auto p = am::v5::basic_unsuback_packet<4>{
        0x12345678,         // packet_id
        args,
        props
    };

    BOOST_TEST(p.packet_id() == 0x12345678);
    BOOST_TEST((p.entries() == args));
    {
        auto cbs = p.const_buffer_sequence();
        BOOST_TEST(cbs.size() == p.num_of_const_buffer_sequence());
        char expected[] {
            char(0xb0),                                     // fixed_header
            0x15,                                           // remaining_length
            0x12, 0x34, 0x56, 0x78,                         // packet_id
            0x0e,                                           // property_length
            0x1f,                                           // reason_string
            0x00, 0x0b, 0x73, 0x6f, 0x6d, 0x65, 0x20, 0x72, 0x65, 0x61, 0x73, 0x6f, 0x6e,
            0x11,                                           // unsuback_reason_code1
            char(0x80)                                      // unsuback_reason_code2
        };
        auto [b, e] = am::make_packet_range(cbs);
        BOOST_TEST(std::equal(b, e, std::begin(expected)));

        am::buffer buf{std::begin(expected), std::end(expected)};
        am::error_code ec;
        auto p = am::v5::basic_unsuback_packet<4>{buf, ec};
        BOOST_TEST(!ec);
        BOOST_TEST(p.packet_id() == 0x12345678);
        BOOST_TEST((p.entries() == args));

        auto cbs2 = p.const_buffer_sequence();
        BOOST_TEST(cbs2.size() == p.num_of_const_buffer_sequence());
        auto [b2, e2] = am::make_packet_range(cbs2);
        BOOST_TEST(std::equal(b2, e2, std::begin(expected)));
    }
    BOOST_TEST(
        boost::lexical_cast<std::string>(p) ==
        "v5::unsuback{pid:305419896,[no_subscription_existed,unspecified_error],ps:[{id:reason_string,val:some reason}]}"
    );
}

BOOST_AUTO_TEST_CASE(v5_unsuback_error) {
    {
        am::buffer buf; // empty
        am::error_code ec;
        am::v5::unsuback_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        //                CP
        am::buffer buf{"\x00"sv}; // invalid type
        am::error_code ec;
        am::v5::unsuback_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        //                CP
        am::buffer buf{"\xb0"sv}; // short
        am::error_code ec;
        am::v5::unsuback_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        //                CP  RL
        am::buffer buf{"\xb0\x01"sv}; // remaining length buf mismatch
        am::error_code ec;
        am::v5::unsuback_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        //                CP  RL
        am::buffer buf{"\xb0\x03"sv}; // invalid remaining length
        am::error_code ec;
        am::v5::unsuback_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        //                CP  RL  PID
        am::buffer buf{"\xb0\x01\x12"sv}; // short pid
        am::error_code ec;
        am::v5::unsuback_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        //                CP  RL  PID
        am::buffer buf{"\xb0\x00\x12\x34"sv}; // zero remaining length
        am::error_code ec;
        am::v5::unsuback_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        //                CP  RL  PID
        am::buffer buf{"\xb0\x02\x12\x34"sv}; // no property length
        am::error_code ec;
        am::v5::unsuback_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        //                CP  RL  PID     PL
        am::buffer buf{"\xb0\x03\x12\x34\x80"sv}; // property length short
        am::error_code ec;
        am::v5::unsuback_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        //                CP  RL  PID     PL
        am::buffer buf{"\xb0\x03\x12\x34\x01"sv}; // property length mismatch
        am::error_code ec;
        am::v5::unsuback_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        //                CP  RL  PID     PL
        am::buffer buf{"\xb0\x03\x12\x34\x00"sv}; // no entry
        am::error_code ec;
        am::v5::unsuback_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        //                CP  RL  PID     PL  RC
        am::buffer buf{"\xb0\x04\x12\x34\x00\xa0"sv}; // invalid rc
        am::error_code ec;
        am::v5::unsuback_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
}

BOOST_AUTO_TEST_SUITE_END()
