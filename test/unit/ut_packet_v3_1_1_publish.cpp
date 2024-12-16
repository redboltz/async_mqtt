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
struct v311_publish;
struct v311_publish_qos0;
struct v311_publish_invalid;
struct v311_publish_pid4;
struct v311_publish_error;
BOOST_AUTO_TEST_SUITE_END()

#include <async_mqtt/packet/v3_1_1_publish.hpp>
#include <async_mqtt/packet/packet_iterator.hpp>
#include <async_mqtt/packet/packet_traits.hpp>

BOOST_AUTO_TEST_SUITE(ut_packet)

namespace am = async_mqtt;
using namespace std::literals::string_view_literals;

BOOST_AUTO_TEST_CASE(v311_publish) {
    BOOST_TEST(am::is_publish<am::v3_1_1::publish_packet>());
    BOOST_TEST(am::is_v3_1_1<am::v3_1_1::publish_packet>());
    BOOST_TEST(!am::is_v5<am::v3_1_1::publish_packet>());
    BOOST_TEST(am::is_client_sendable<am::v3_1_1::publish_packet>());
    BOOST_TEST(am::is_server_sendable<am::v3_1_1::publish_packet>());

    auto p = am::v3_1_1::publish_packet{
        0x1234, // packet_id
        "topic1",
        "payload1",
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes
    };

    BOOST_TEST(p.packet_id() == 0x1234);
    BOOST_TEST(p.topic() == "topic1");
    {
        auto const& bs = p.payload_as_buffer();
        auto [b, e] = am::make_packet_range(bs);
        std::string_view expected = "payload1";
        BOOST_TEST(std::equal(b, e, expected.begin()));
    }
    BOOST_TEST(p.opts().get_qos() == am::qos::exactly_once);
    BOOST_TEST(p.opts().get_retain() == am::pub::retain::yes);
    BOOST_TEST(p.opts().get_dup() == am::pub::dup::yes);
    p.set_dup(false);
    BOOST_TEST(p.opts().get_dup() == am::pub::dup::no);

    {
        auto cbs = p.const_buffer_sequence();
        BOOST_TEST(cbs.size() == p.num_of_const_buffer_sequence());
        char expected[] {
            0x35,                               // fixed_header
            0x12,                               // remaining_length
            0x00, 0x06,                         // topic_name_length
            0x74, 0x6f, 0x70, 0x69, 0x63, 0x31, // topic_name
            0x12, 0x34,                         // packet_id
            0x70, 0x61, 0x79, 0x6c, 0x6f, 0x61, 0x64, 0x31 // payload
        };
        auto [b, e] = am::make_packet_range(cbs);
        BOOST_TEST(std::equal(b, e, std::begin(expected)));

        am::buffer buf{std::begin(expected), std::end(expected)};
        am::error_code ec;
        auto p = am::v3_1_1::publish_packet{buf, ec};
        BOOST_TEST(!ec);
        BOOST_TEST(p.packet_id() == 0x1234);
        BOOST_TEST(p.topic() == "topic1");
        {
            auto const& bs = p.payload_as_buffer();
            auto [b, e] = am::make_packet_range(bs);
            std::string_view expected = "payload1";
            BOOST_TEST(std::equal(b, e, expected.begin()));
        }
        BOOST_TEST(p.opts().get_qos() == am::qos::exactly_once);
        BOOST_TEST(p.opts().get_retain() == am::pub::retain::yes);
        BOOST_TEST(p.opts().get_dup() == am::pub::dup::no);

        auto cbs2 = p.const_buffer_sequence();
        BOOST_TEST(cbs2.size() == p.num_of_const_buffer_sequence());
        auto [b2, e2] = am::make_packet_range(cbs2);
        BOOST_TEST(std::equal(b2, e2, std::begin(expected)));

        BOOST_TEST(p.type() == am::control_packet_type::publish);
    }
#if defined(ASYNC_MQTT_PRINT_PAYLOAD)
    BOOST_TEST(
        boost::lexical_cast<std::string>(p) ==
        "v3_1_1::publish{topic:topic1,qos:exactly_once,retain:yes,dup:no,pid:4660,payload:payload1}"
    );
#else  // defined(ASYNC_MQTT_PRINT_PAYLOAD)
    BOOST_TEST(
        boost::lexical_cast<std::string>(p) ==
        "v3_1_1::publish{topic:topic1,qos:exactly_once,retain:yes,dup:no,pid:4660}"
    );
#endif // defined(ASYNC_MQTT_PRINT_PAYLOAD)

    auto p2 = am::v3_1_1::publish_packet{
        0x1234, // packet_id
        "topic1",
        "payload1",
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::no
    };

    auto p3 = am::v3_1_1::publish_packet{
        0x1235, // packet_id
        "topic1",
        "payload1",
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::no
    };

    BOOST_CHECK(p == p2);
    BOOST_CHECK(!(p < p2));
    BOOST_CHECK(!(p2< p));
    BOOST_CHECK(p < p3 || p3 < p);

    try {
        auto p = am::v3_1_1::publish_packet{
            0x1234, // packet_id
            "topic1",
            "payload1",
            static_cast<am::pub::opts>(0x06) // QoS3 ?
        };
        BOOST_TEST(false);
    }
    catch (am::system_error const& se) {
        BOOST_TEST(se.code() == am::disconnect_reason_code::malformed_packet);
    }

    try {
        auto p = am::v3_1_1::publish_packet{
            0x1234, // packet_id
            std::string(0x10000, 'w'), // too long
            "payload1",
            static_cast<am::pub::opts>(0x03) // QoS3 ?
        };
        BOOST_TEST(false);
    }
    catch (am::system_error const& se) {
        BOOST_TEST(se.code() == am::disconnect_reason_code::malformed_packet);
    }

}

BOOST_AUTO_TEST_CASE(v311_publish_qos0) {
    auto p = am::v3_1_1::publish_packet{
        "topic1",
        "payload1",
        am::qos::at_most_once | am::pub::retain::yes | am::pub::dup::yes
    };

    BOOST_TEST(p.packet_id() == 0);
    BOOST_TEST(p.topic() == "topic1");
    {
        auto const& bs = p.payload_as_buffer();
        auto [b, e] = am::make_packet_range(bs);
        std::string_view expected = "payload1";
        BOOST_TEST(std::equal(b, e, expected.begin()));
    }
    BOOST_TEST(p.opts().get_qos() == am::qos::at_most_once);
    BOOST_TEST(p.opts().get_retain() == am::pub::retain::yes);
    BOOST_TEST(p.opts().get_dup() == am::pub::dup::yes);
    p.set_dup(false);
    BOOST_TEST(p.opts().get_dup() == am::pub::dup::no);

    {
        auto cbs = p.const_buffer_sequence();
        BOOST_TEST(cbs.size() == p.num_of_const_buffer_sequence());
        char expected[] {
            0x31,                               // fixed_header
            0x10,                               // remaining_length
            0x00, 0x06,                         // topic_name_length
            0x74, 0x6f, 0x70, 0x69, 0x63, 0x31, // topic_name
            0x70, 0x61, 0x79, 0x6c, 0x6f, 0x61, 0x64, 0x31 // payload
        };
        auto [b, e] = am::make_packet_range(cbs);
        BOOST_TEST(std::equal(b, e, std::begin(expected)));

        am::buffer buf{std::begin(expected), std::end(expected)};
        am::error_code ec;
        auto p = am::v3_1_1::publish_packet{buf, ec};
        BOOST_TEST(!ec);
        BOOST_TEST(p.packet_id() == 0);
        BOOST_TEST(p.topic() == "topic1");
        {
            auto const& bs = p.payload_as_buffer();
            auto [b, e] = am::make_packet_range(bs);
            std::string_view expected = "payload1";
            BOOST_TEST(std::equal(b, e, expected.begin()));
        }
        BOOST_TEST(p.opts().get_qos() == am::qos::at_most_once);
        BOOST_TEST(p.opts().get_retain() == am::pub::retain::yes);
        BOOST_TEST(p.opts().get_dup() == am::pub::dup::no);

        auto cbs2 = p.const_buffer_sequence();
        BOOST_TEST(cbs2.size() == p.num_of_const_buffer_sequence());
        auto [b2, e2] = am::make_packet_range(cbs2);
        BOOST_TEST(std::equal(b2, e2, std::begin(expected)));
    }
#if defined(ASYNC_MQTT_PRINT_PAYLOAD)
    BOOST_TEST(
        boost::lexical_cast<std::string>(p) ==
        "v3_1_1::publish{topic:topic1,qos:at_most_once,retain:yes,dup:no,payload:payload1}"
    );
#else  // defined(ASYNC_MQTT_PRINT_PAYLOAD)
    BOOST_TEST(
        boost::lexical_cast<std::string>(p) ==
        "v3_1_1::publish{topic:topic1,qos:at_most_once,retain:yes,dup:no}"
    );
#endif // defined(ASYNC_MQTT_PRINT_PAYLOAD)
}

BOOST_AUTO_TEST_CASE(v311_publish_invalid) {
    try {
        auto p = am::v3_1_1::publish_packet{
            "topic1",
            "payload1",
            am::qos::at_least_once | am::pub::retain::yes | am::pub::dup::yes
        };
        BOOST_TEST(false);
    }
    catch (am::system_error const& se) {
        BOOST_TEST(se.code() == am::disconnect_reason_code::malformed_packet);
    }
    try {
        auto p = am::v3_1_1::publish_packet{
            1,
            "topic1",
            "payload1",
            am::qos::at_most_once | am::pub::retain::yes | am::pub::dup::yes
        };
        BOOST_TEST(false);
    }
    catch (am::system_error const& se) {
        BOOST_TEST(se.code() == am::disconnect_reason_code::malformed_packet);
    }
}

BOOST_AUTO_TEST_CASE(v311_publish_pid4) {
    auto p = am::v3_1_1::basic_publish_packet<4>{
        0x12345678, // packet_id
        "topic1",
        "payload1",
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes
    };

    BOOST_TEST(p.packet_id() == 0x12345678);
    BOOST_TEST(p.topic() == "topic1");
    {
        auto const& bs = p.payload_as_buffer();
        auto [b, e] = am::make_packet_range(bs);
        std::string_view expected = "payload1";
        BOOST_TEST(std::equal(b, e, expected.begin()));
    }
    BOOST_TEST(p.opts().get_qos() == am::qos::exactly_once);
    BOOST_TEST(p.opts().get_retain() == am::pub::retain::yes);
    BOOST_TEST(p.opts().get_dup() == am::pub::dup::yes);
    p.set_dup(false);
    BOOST_TEST(p.opts().get_dup() == am::pub::dup::no);

    {
        auto cbs = p.const_buffer_sequence();
        BOOST_TEST(cbs.size() == p.num_of_const_buffer_sequence());
        char expected[] {
            0x35,                               // fixed_header
            0x14,                               // remaining_length
            0x00, 0x06,                         // topic_name_length
            0x74, 0x6f, 0x70, 0x69, 0x63, 0x31, // topic_name
            0x12, 0x34, 0x56, 0x78,             // packet_id
            0x70, 0x61, 0x79, 0x6c, 0x6f, 0x61, 0x64, 0x31 // payload
        };
        auto [b, e] = am::make_packet_range(cbs);
        BOOST_TEST(std::equal(b, e, std::begin(expected)));

        am::buffer buf{std::begin(expected), std::end(expected)};
        am::error_code ec;
        auto p = am::v3_1_1::basic_publish_packet<4>{buf, ec};
        BOOST_TEST(!ec);
        BOOST_TEST(p.packet_id() == 0x12345678);
        BOOST_TEST(p.topic() == "topic1");
        {
            auto const& bs = p.payload_as_buffer();
            auto [b, e] = am::make_packet_range(bs);
            std::string_view expected = "payload1";
            BOOST_TEST(std::equal(b, e, expected.begin()));
        }
        BOOST_TEST(p.opts().get_qos() == am::qos::exactly_once);
        BOOST_TEST(p.opts().get_retain() == am::pub::retain::yes);
        BOOST_TEST(p.opts().get_dup() == am::pub::dup::no);

        auto cbs2 = p.const_buffer_sequence();
        BOOST_TEST(cbs2.size() == p.num_of_const_buffer_sequence());
        auto [b2, e2] = am::make_packet_range(cbs2);
        BOOST_TEST(std::equal(b2, e2, std::begin(expected)));
    }
#if defined(ASYNC_MQTT_PRINT_PAYLOAD)
    BOOST_TEST(
        boost::lexical_cast<std::string>(p) ==
        "v3_1_1::publish{topic:topic1,qos:exactly_once,retain:yes,dup:no,pid:305419896,payload:payload1}"
    );
#else  // defined(ASYNC_MQTT_PRINT_PAYLOAD)
    BOOST_TEST(
        boost::lexical_cast<std::string>(p) ==
        "v3_1_1::publish{topic:topic1,qos:exactly_once,retain:yes,dup:no,pid:305419896}"
    );
#endif // defined(ASYNC_MQTT_PRINT_PAYLOAD)
}

BOOST_AUTO_TEST_CASE(v311_publish_error) {
    {
        am::buffer buf; // empty
        am::error_code ec;
        am::v3_1_1::publish_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        //                CP
        am::buffer buf{"\x00"sv}; // invalid type
        am::error_code ec;
        am::v3_1_1::publish_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        //                CP
        am::buffer buf{"\x32"sv}; // short
        am::error_code ec;
        am::v3_1_1::publish_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        //                CP  RL
        am::buffer buf{"\x32\x01"sv}; // remaining length buf mismatch
        am::error_code ec;
        am::v3_1_1::publish_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        //                CP  RL
        am::buffer buf{"\x32\x03"sv}; // invalid remaining length
        am::error_code ec;
        am::v3_1_1::publish_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        //                CP  RL  TPL
        am::buffer buf{"\x32\x01\x00"sv}; // short topic name length
        am::error_code ec;
        am::v3_1_1::publish_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        //                CP  RL  TPL
        am::buffer buf{"\x32\x02\x02T"sv}; // mismatch topic name length
        am::error_code ec;
        am::v3_1_1::publish_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        //                CP  RL  TPL
        am::buffer buf{"\x32\x00\x00\x00"sv}; // zero remaining length
        am::error_code ec;
        am::v3_1_1::publish_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        //                CP  RL  TPL
        am::buffer buf{"\x30\x02\x00\x00"sv}; // no pid QoS0 valid
        am::error_code ec;
        am::v3_1_1::publish_packet{buf, ec};
        BOOST_TEST(ec == am::error_code{});
    }
    {
        //                CP  RL  TPL
        am::buffer buf{"\x36\x02\x00\x00"sv}; // QoS3 invalid
        am::error_code ec;
        am::v3_1_1::publish_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        //                CP  RL  TPL
        am::buffer buf{"\x32\x02\x00\x00"sv}; // no pid QoS1 invalid
        am::error_code ec;
        am::v3_1_1::publish_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        //                CP  RL  TPL   TP PID
        am::buffer buf{"\x32\x05\x00\x01T\x12\x34"sv}; // no payload (valid)
        am::error_code ec;
        am::v3_1_1::publish_packet{buf, ec};
        BOOST_TEST(ec == am::error_code{});
    }
    {
        //                CP  RL  TPL     TP      PID
        am::buffer buf{"\x32\x06\x00\x02\xc2\xc0\x12\x34"sv}; // invalid utf8 topic
        am::error_code ec;
        am::v3_1_1::publish_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        //                CP  RL  TPL   TP PID
        am::buffer buf{"\x32\x04\x00\x01T\x12"sv}; // invalid pid
        am::error_code ec;
        am::v3_1_1::publish_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        //                CP  RL  TPL   TP PID
        am::buffer buf{"\x32\x05\x00\x01T\x12\x34\x00"sv}; // too long
        am::error_code ec;
        am::v3_1_1::publish_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
}

BOOST_AUTO_TEST_SUITE_END()
