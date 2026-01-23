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
struct v5_unsubscribe;
struct v5_unsubscribe_pid4;
struct v5_unsubscribe_error;
BOOST_AUTO_TEST_SUITE_END()

#include <async_mqtt/protocol/packet/v5_unsubscribe.hpp>
#include <async_mqtt/protocol/packet/packet_iterator.hpp>
#include <async_mqtt/protocol/packet/packet_traits.hpp>

BOOST_AUTO_TEST_SUITE(ut_packet)

namespace am = async_mqtt;
using namespace std::literals::string_view_literals;

BOOST_AUTO_TEST_CASE(v5_unsubscribe) {
    BOOST_TEST(am::is_unsubscribe<am::v5::unsubscribe_packet>());
    BOOST_TEST(!am::is_v3_1_1<am::v5::unsubscribe_packet>());
    BOOST_TEST(am::is_v5<am::v5::unsubscribe_packet>());
    BOOST_TEST(am::is_client_sendable<am::v5::unsubscribe_packet>());
    BOOST_TEST(!am::is_server_sendable<am::v5::unsubscribe_packet>());

    std::vector<am::topic_sharename> args {
        "topic1",
        "topic2",
    };

    auto props = am::properties{
        am::property::user_property("mykey", "myval")
    };
    auto p = am::v5::unsubscribe_packet{
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
            char(0xa2),                                     // fixed_header
            0x22,                                           // remaining_length
            0x12, 0x34,                                     // packet_id
            0x0f,                                           // property_length
            0x26,                                           // user_property
            0x00, 0x05, 0x6d, 0x79, 0x6b, 0x65, 0x79,       // mykey
            0x00, 0x05, 0x6d, 0x79, 0x76, 0x61, 0x6c,       // myval
            0x00, 0x06, 0x74, 0x6f, 0x70, 0x69, 0x63, 0x31, // topic1
            0x00, 0x06, 0x74, 0x6f, 0x70, 0x69, 0x63, 0x32, // topic2
        };
        auto [b, e] = am::make_packet_range(cbs);
        BOOST_TEST(std::equal(b, e, std::begin(expected)));

        am::buffer buf{std::begin(expected), std::end(expected)};
        am::error_code ec;
        auto p = am::v5::unsubscribe_packet{buf, ec};
        BOOST_TEST(!ec);
        BOOST_TEST(p.packet_id() == 0x1234);
        BOOST_TEST((p.entries() == args));
        BOOST_TEST(p.props() == props);

        auto cbs2 = p.const_buffer_sequence();
        BOOST_TEST(cbs2.size() == p.num_of_const_buffer_sequence());
        auto [b2, e2] = am::make_packet_range(cbs2);
        BOOST_TEST(std::equal(b2, e2, std::begin(expected)));

        BOOST_TEST(p.type() == am::control_packet_type::unsubscribe);
    }
    BOOST_TEST(
        boost::lexical_cast<std::string>(p) ==
        "v5::unsubscribe{pid:4660,[{topic:topic1,sn:},{topic:topic2,sn:}],ps:[{id:user_property,key:mykey,val:myval}]}"
    );

    auto p2 = am::v5::unsubscribe_packet{
        0x1234,         // packet_id
        args,
        props
    };

    auto p3 = am::v5::unsubscribe_packet{
        0x1235,         // packet_id
        args,
        props
    };

    BOOST_CHECK(p == p2);
    BOOST_CHECK(!(p < p2));
    BOOST_CHECK(!(p2< p));
    BOOST_CHECK(p < p3 || p3 < p);

    try {
        std::vector<am::topic_sharename> args {
            {
                std::string(0x10000, 'w') // too long
            }
        };

        auto p = am::v5::unsubscribe_packet{
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
        auto p = am::v5::unsubscribe_packet{
            0x1234,         // packet_id
            args,
            props
        };
    }
    catch (am::system_error const& se) {
        BOOST_TEST(se.code() == am::disconnect_reason_code::malformed_packet);
    }

    try {
        auto p = am::v5::unsubscribe_packet{
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

    // ShareName must not contain "+"
    try {
        std::vector<am::topic_sharename> args {
            "$share/sn+/topic"
        };

        auto p = am::v5::unsubscribe_packet{
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
        std::vector<am::topic_sharename> args {
            "$share/sn#/topic"
        };

        auto p = am::v5::unsubscribe_packet{
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

BOOST_AUTO_TEST_CASE(v5_unsubscribe_pid4) {
    std::vector<am::topic_sharename> args {
        "topic1",
        "topic2",
    };

    auto props = am::properties{
        am::property::user_property("mykey", "myval")
    };
    auto p = am::v5::basic_unsubscribe_packet<4>{
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
            char(0xa2),                                     // fixed_header
            0x24,                                           // remaining_length
            0x12, 0x34, 0x56, 0x78,                         // packet_id
            0x0f,                                           // property_length
            0x26,                                           // user_property
            0x00, 0x05, 0x6d, 0x79, 0x6b, 0x65, 0x79,       // mykey
            0x00, 0x05, 0x6d, 0x79, 0x76, 0x61, 0x6c,       // myval
            0x00, 0x06, 0x74, 0x6f, 0x70, 0x69, 0x63, 0x31, // topic1
            0x00, 0x06, 0x74, 0x6f, 0x70, 0x69, 0x63, 0x32, // topic2
        };
        auto [b, e] = am::make_packet_range(cbs);
        BOOST_TEST(std::equal(b, e, std::begin(expected)));

        am::buffer buf{std::begin(expected), std::end(expected)};
        am::error_code ec;
        auto p = am::v5::basic_unsubscribe_packet<4>{buf, ec};
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
        "v5::unsubscribe{pid:305419896,[{topic:topic1,sn:},{topic:topic2,sn:}],ps:[{id:user_property,key:mykey,val:myval}]}"
    );
}

BOOST_AUTO_TEST_CASE(v5_unsubscribe_error) {
    {
        am::buffer buf; // empty
        am::error_code ec;
        am::v5::unsubscribe_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        //                CP
        am::buffer buf{"\x00"sv}; // invalid type
        am::error_code ec;
        am::v5::unsubscribe_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        //                CP
        am::buffer buf{"\xa2"sv}; // short
        am::error_code ec;
        am::v5::unsubscribe_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        //                CP  RL
        am::buffer buf{"\xa2\x01"sv}; // remaining length buf mismatch
        am::error_code ec;
        am::v5::unsubscribe_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        //                CP  RL
        am::buffer buf{"\xa2\x03"sv}; // invalid remaining length
        am::error_code ec;
        am::v5::unsubscribe_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        //                CP  RL  PID
        am::buffer buf{"\xa2\x01\x12"sv}; // short pid
        am::error_code ec;
        am::v5::unsubscribe_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        //                CP  RL  PID
        am::buffer buf{"\xa2\x00\x12\x34"sv}; // zero remaining length
        am::error_code ec;
        am::v5::unsubscribe_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        //                CP  RL  PID
        am::buffer buf{"\xa2\x02\x12\x34"sv}; // no property elngth
        am::error_code ec;
        am::v5::unsubscribe_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        //                CP  RL  PID     PL
        am::buffer buf{"\xa2\x03\x12\x34\x80"sv}; // property length short
        am::error_code ec;
        am::v5::unsubscribe_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        //                CP  RL  PID     PL
        am::buffer buf{"\xa2\x03\x12\x34\x01"sv}; // property length mismatch
        am::error_code ec;
        am::v5::unsubscribe_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        //                CP  RL  PID     PL
        am::buffer buf{"\xa2\x03\x12\x34\x00"sv}; // no entry
        am::error_code ec;
        am::v5::unsubscribe_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
    }
    {
        //                CP  RL  PID     PL TPL
        am::buffer buf{"\xa2\x04\x12\x34\x00\x00"sv}; // invalid topic len
        am::error_code ec;
        am::v5::unsubscribe_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        //                CP  RL  PID     PL  TPL
        am::buffer buf{"\xa2\x05\x12\x34\x00\x00\x01"sv}; // topic len but mismatch
        am::error_code ec;
        am::v5::unsubscribe_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        //                CP  RL  PID     PL  TPL    TP
        am::buffer buf{"\xa2\x07\x12\x34\x00\x00\x02\xc0\x00"sv}; // invalid utf8 topic
        am::error_code ec;
        am::v5::unsubscribe_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        //                CP  RL  PID     PL  TPL   TP
        am::buffer buf{"\xa2\x15\x12\x34\x00\x00\x10$share/sn+/topic"sv}; // sharename contains +
        am::error_code ec;
        am::v5::unsubscribe_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        //                CP  RL  PID     PL  TPL   TP
        am::buffer buf{"\xa2\x15\x12\x34\x00\x00\x10$share/sn#/topic"sv}; // sharename contains #
        am::error_code ec;
        am::v5::unsubscribe_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
}

BOOST_AUTO_TEST_SUITE_END()
