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
struct v5_subscribe;
struct v5_subscribe_pid4;
struct v5_subscribe_error;
BOOST_AUTO_TEST_SUITE_END()

#include <async_mqtt/protocol/packet/v5_subscribe.hpp>
#include <async_mqtt/protocol/packet/packet_iterator.hpp>
#include <async_mqtt/protocol/packet/packet_traits.hpp>

BOOST_AUTO_TEST_SUITE(ut_packet)

namespace am = async_mqtt;
using namespace std::literals::string_view_literals;

BOOST_AUTO_TEST_CASE(v5_subscribe) {
    BOOST_TEST(am::is_subscribe<am::v5::subscribe_packet>());
    BOOST_TEST(!am::is_v3_1_1<am::v5::subscribe_packet>());
    BOOST_TEST(am::is_v5<am::v5::subscribe_packet>());
    BOOST_TEST(am::is_client_sendable<am::v5::subscribe_packet>());
    BOOST_TEST(!am::is_server_sendable<am::v5::subscribe_packet>());

    std::vector<am::topic_subopts> args {
        {"topic1", am::qos::at_most_once | am::sub::nl::yes | am::sub::retain_handling::not_send},
        {"topic2", am::qos::exactly_once | am::sub::rap::retain},
    };

    auto props = am::properties{
        am::property::subscription_identifier(0x0fffffff)
    };
    auto p = am::v5::subscribe_packet{
        0x1234,         // packet_id
        args,
        props
    };

    BOOST_TEST(p.packet_id() == 0x1234);
    BOOST_TEST((p.entries() == args));
    BOOST_TEST(p.props() == props);
    {
        auto cbs = p.const_buffer_sequence();
        BOOST_TEST(cbs.size() == p.num_of_const_buffer_sequence());
        char expected[] {
            char(0x82),                                     // fixed_header
            0x1a,                                           // remaining_length
            0x12, 0x34,                                     // packet_id
            0x05,                                           // property_length
            0x0b, char(0xff), char(0xff), char(0xff), 0x7f, // subscription_identifier
            0x00, 0x06, 0x74, 0x6f, 0x70, 0x69, 0x63, 0x31, // topic1
            0x24,                                           // opts1
            0x00, 0x06, 0x74, 0x6f, 0x70, 0x69, 0x63, 0x32, // topic2
            0x0a                                            // opts2
        };
        auto [b, e] = am::make_packet_range(cbs);
        BOOST_TEST(std::equal(b, e, std::begin(expected)));

        am::buffer buf{std::begin(expected), std::end(expected)};
        am::error_code ec;
        auto p = am::v5::subscribe_packet{buf, ec};
        BOOST_TEST(!ec);
        BOOST_TEST(p.packet_id() == 0x1234);
        BOOST_TEST((p.entries() == args));
        BOOST_TEST(p.props() == props);

        auto cbs2 = p.const_buffer_sequence();
        BOOST_TEST(cbs2.size() == p.num_of_const_buffer_sequence());
        auto [b2, e2] = am::make_packet_range(cbs2);
        BOOST_TEST(std::equal(b2, e2, std::begin(expected)));

        BOOST_TEST(p.type() == am::control_packet_type::subscribe);
    }
    BOOST_TEST(
        boost::lexical_cast<std::string>(p) ==
        "v5::subscribe{pid:4660,[{topic:topic1,sn:,qos:at_most_once,rh:not_send,nl:yes,rap:dont},{topic:topic2,sn:,qos:exactly_once,rh:send,nl:no,rap:retain}],ps:[{id:subscription_identifier,val:268435455}]}"
    );

    auto p2 = am::v5::subscribe_packet{
        0x1234,         // packet_id
        args,
        props
    };
    auto p3 = am::v5::subscribe_packet{
        0x1235,         // packet_id
        args,
        props
    };
    BOOST_CHECK(p == p2);
    BOOST_CHECK(!(p < p2));
    BOOST_CHECK(!(p2< p));
    BOOST_CHECK(p < p3 || p3 < p);

    try {
        std::vector<am::topic_subopts> args {
            {"topic1", static_cast<am::sub::opts>(0x80)},
        };

        auto p = am::v5::subscribe_packet{
            0x1234,         // packet_id
            args,
            props
        };
        BOOST_TEST(false);
    }
    catch (am::system_error const& se) {
        BOOST_TEST(se.code() == am::disconnect_reason_code::malformed_packet);
    }

    try {
        std::vector<am::topic_subopts> args {
            {"topic1", static_cast<am::sub::opts>(0x03)}, // QoS 3?
        };

        auto p = am::v5::subscribe_packet{
            0x1234,         // packet_id
            args,
            props
        };
        BOOST_TEST(false);
    }
    catch (am::system_error const& se) {
        BOOST_TEST(se.code() == am::disconnect_reason_code::malformed_packet);
    }

    try {
        std::vector<am::topic_subopts> args {
            {"topic1", static_cast<am::sub::opts>(0x30)}, // invalid retain handling
        };

        auto p = am::v5::subscribe_packet{
            0x1234,         // packet_id
            args,
            props
        };
        BOOST_TEST(false);
    }
    catch (am::system_error const& se) {
        BOOST_TEST(se.code() == am::disconnect_reason_code::malformed_packet);
    }

    try {
        std::vector<am::topic_subopts> args {
            {
                std::string(0x10000, 'w'), // too long
                am::qos::at_most_once
            }
        };

        auto p = am::v5::subscribe_packet{
            0x1234,         // packet_id
            args,
            props
        };
        BOOST_TEST(false);
    }
    catch (am::system_error const& se) {
        BOOST_TEST(se.code() == am::disconnect_reason_code::malformed_packet);
    }

    try {
        auto props = am::properties{
            am::property::will_delay_interval{1}
        };
        auto p = am::v5::subscribe_packet{
            0x1234,         // packet_id
            args,
            props
        };
    }
    catch (am::system_error const& se) {
        BOOST_TEST(se.code() == am::disconnect_reason_code::malformed_packet);
    }

    try {
        std::vector<am::topic_subopts> args {
            {
                std::string("$share/topic1"),
                am::sub::nl::yes
            }
        };

        auto p = am::v5::subscribe_packet{
            0x1234,         // packet_id
            args,
            props
        };
        BOOST_TEST(false);
    }
    catch (am::system_error const& se) {
        BOOST_TEST(se.code() == am::disconnect_reason_code::protocol_error);
    }

    // ShareName must not contain "+"
    try {
        std::vector<am::topic_subopts> args {
            {
                std::string("$share/sn+/topic"),
                am::qos::at_most_once
            }
        };

        auto p = am::v5::subscribe_packet{
            0x1234,         // packet_id
            args,
            am::properties{}
        };
        BOOST_TEST(false);
    }
    catch (am::system_error const& se) {
        BOOST_TEST(se.code() == am::disconnect_reason_code::malformed_packet);
    }

    // ShareName must not contain "#"
    try {
        std::vector<am::topic_subopts> args {
            {
                std::string("$share/sn#/topic"),
                am::qos::at_most_once
            }
        };

        auto p = am::v5::subscribe_packet{
            0x1234,         // packet_id
            args,
            am::properties{}
        };
        BOOST_TEST(false);
    }
    catch (am::system_error const& se) {
        BOOST_TEST(se.code() == am::disconnect_reason_code::malformed_packet);
    }

}

BOOST_AUTO_TEST_CASE(v5_subscribe_pid4) {
    std::vector<am::topic_subopts> args {
        {"topic1", am::qos::at_most_once | am::sub::nl::yes | am::sub::retain_handling::not_send},
        {"topic2", am::qos::exactly_once | am::sub::rap::retain},
    };

    auto props = am::properties{
        am::property::subscription_identifier(0x0fffffff)
    };
    auto p = am::v5::basic_subscribe_packet<4>{
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
            char(0x82),                                     // fixed_header
            0x1c,                                           // remaining_length
            0x12, 0x34, 0x56, 0x78,                         // packet_id
            0x05,                                           // property_length
            0x0b, char(0xff), char(0xff), char(0xff), 0x7f, // subscription_identifier
            0x00, 0x06, 0x74, 0x6f, 0x70, 0x69, 0x63, 0x31, // topic1
            0x24,                                           // opts1
            0x00, 0x06, 0x74, 0x6f, 0x70, 0x69, 0x63, 0x32, // topic2
            0x0a                                            // opts2
        };
        auto [b, e] = am::make_packet_range(cbs);
        BOOST_TEST(std::equal(b, e, std::begin(expected)));

        am::buffer buf{std::begin(expected), std::end(expected)};
        am::error_code ec;
        auto p = am::v5::basic_subscribe_packet<4>{buf, ec};
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
        "v5::subscribe{pid:305419896,[{topic:topic1,sn:,qos:at_most_once,rh:not_send,nl:yes,rap:dont},{topic:topic2,sn:,qos:exactly_once,rh:send,nl:no,rap:retain}],ps:[{id:subscription_identifier,val:268435455}]}"
    );
}

BOOST_AUTO_TEST_CASE(v5_subscribe_error) {
    {
        am::buffer buf; // empty
        am::error_code ec;
        am::v5::subscribe_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        //                CP
        am::buffer buf{"\x00"sv}; // invalid type
        am::error_code ec;
        am::v5::subscribe_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        //                CP
        am::buffer buf{"\x82"sv}; // short
        am::error_code ec;
        am::v5::subscribe_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        //                CP  RL
        am::buffer buf{"\x82\x01"sv}; // remaining length buf mismatch
        am::error_code ec;
        am::v5::subscribe_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        //                CP  RL
        am::buffer buf{"\x82\x03"sv}; // invalid remaining length
        am::error_code ec;
        am::v5::subscribe_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        //                CP  RL  PID
        am::buffer buf{"\x82\x01\x12"sv}; // short pid
        am::error_code ec;
        am::v5::subscribe_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        //                CP  RL  PID
        am::buffer buf{"\x82\x00\x12\x34"sv}; // zero remaining length
        am::error_code ec;
        am::v5::subscribe_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        //                CP  RL  PID
        am::buffer buf{"\x82\x02\x12\x34"sv}; // no property length
        am::error_code ec;
        am::v5::subscribe_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        //                CP  RL  PID     PL
        am::buffer buf{"\x82\x03\x12\x34\x80"sv}; // property length short
        am::error_code ec;
        am::v5::subscribe_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        //                CP  RL  PID     PL
        am::buffer buf{"\x82\x03\x12\x34\x01"sv}; // property length mismatch
        am::error_code ec;
        am::v5::subscribe_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        //                CP  RL  PID     PL
        am::buffer buf{"\x82\x03\x12\x34\x00"sv}; // no entry
        am::error_code ec;
        am::v5::subscribe_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        //                CP  RL  PID     PL  TPL
        am::buffer buf{"\x82\x04\x12\x34\x00\x00"sv}; // invalid topic len
        am::error_code ec;
        am::v5::subscribe_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        //                CP  RL  PID     PL  TPL
        am::buffer buf{"\x82\x05\x12\x34\x00\x00\x01"sv}; // topic len but mismatch
        am::error_code ec;
        am::v5::subscribe_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        //                CP  RL  PID     PL  TPL   TP OPT
        am::buffer buf{"\x82\x06\x12\x34\x00\x00\x01T"sv}; // no opts
        am::error_code ec;
        am::v5::subscribe_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        //                CP  RL  PID     PL  TPL   TP OPT
        am::buffer buf{"\x82\x07\x12\x34\x00\x00\x01T\x80"sv}; // invalid opts
        am::error_code ec;
        am::v5::subscribe_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        //                CP  RL  PID     PL  TPL   TP OPT
        am::buffer buf{"\x82\x07\x12\x34\x00\x00\x01T\x03"sv}; // invalid opts qos3
        am::error_code ec;
        am::v5::subscribe_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        //                CP  RL  PID     PL  TPL   TP OPT
        am::buffer buf{"\x82\x07\x12\x34\x00\x00\x01T\x00"sv}; // valid
        am::error_code ec;
        am::v5::subscribe_packet{buf, ec};
        BOOST_TEST(ec == am::errc::success);
    }
    {
        //                CP  RL  PID     PL  TPL     TP      OPT
        am::buffer buf{"\x82\x08\x12\x34\x00\x00\x02\xc2\xc0\x00"sv}; // invalid utf8 topic
        am::error_code ec;
        am::v5::subscribe_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        //                CP  RL  PID     PL  TPL   TP      OPT
        am::buffer buf{"\x82\x0e\x12\x34\x00\x00\x08$share/T\x04"sv}; // invalid sharename + nl
        am::error_code ec;
        am::v5::subscribe_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        //                CP  RL  PID     PL  TPL   TP               OPT
        am::buffer buf{"\x82\x16\x12\x34\x00\x00\x10$share/sn+/topic\x00"sv}; // sharename contains +
        am::error_code ec;
        am::v5::subscribe_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        //                CP  RL  PID     PL  TPL   TP               OPT
        am::buffer buf{"\x82\x16\x12\x34\x00\x00\x10$share/sn#/topic\x00"sv}; // sharename contains #
        am::error_code ec;
        am::v5::subscribe_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
}

BOOST_AUTO_TEST_SUITE_END()
