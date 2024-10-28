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
struct v5_publish;
struct v5_publish_qos0;
struct v5_publish_invalid;
struct v5_publish_pid4;
struct v5_publish_topic_alias;
struct v5_publish_error;
BOOST_AUTO_TEST_SUITE_END()

#include <async_mqtt/protocol/packet/v5_publish.hpp>
#include <async_mqtt/protocol/packet/packet_iterator.hpp>
#include <async_mqtt/protocol/packet/packet_traits.hpp>

BOOST_AUTO_TEST_SUITE(ut_packet)

namespace am = async_mqtt;
using namespace std::literals::string_view_literals;

BOOST_AUTO_TEST_CASE(v5_publish) {
    BOOST_TEST(am::is_publish<am::v5::publish_packet>());
    BOOST_TEST(!am::is_v3_1_1<am::v5::publish_packet>());
    BOOST_TEST(am::is_v5<am::v5::publish_packet>());
    BOOST_TEST(am::is_client_sendable<am::v5::publish_packet>());
    BOOST_TEST(am::is_server_sendable<am::v5::publish_packet>());

    auto p = am::v5::publish_packet{
        0x1234, // packet_id
        "topic1",
        "payload1",
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{
            am::property::content_type("json")
        }
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
            0x1a,                               // remaining_length
            0x00, 0x06,                         // topic_name_length
            0x74, 0x6f, 0x70, 0x69, 0x63, 0x31, // topic_name
            0x12, 0x34,                         // packet_id
            0x07,                               // property_length
            0x03,                               // content_type
            0x00, 0x04, 0x6a, 0x73, 0x6f, 0x6e, // json
            0x70, 0x61, 0x79, 0x6c, 0x6f, 0x61, 0x64, 0x31 // payload
        };
        auto [b, e] = am::make_packet_range(cbs);
        BOOST_TEST(std::equal(b, e, std::begin(expected)));

        am::buffer buf{std::begin(expected), std::end(expected)};
        am::error_code ec;
        auto p = am::v5::publish_packet{buf, ec};
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

        BOOST_TEST(p.type() == am::control_packet_type::publish);
    }

#if defined(ASYNC_MQTT_PRINT_PAYLOAD)
    BOOST_TEST(
        boost::lexical_cast<std::string>(p) ==
        "v5::publish{topic:topic1,qos:exactly_once,retain:yes,dup:no,pid:4660,payload:payload1,ps:[{id:content_type,val:json}]}"
    );
#else  // defined(ASYNC_MQTT_PRINT_PAYLOAD)
    BOOST_TEST(
        boost::lexical_cast<std::string>(p) ==
        "v5::publish{topic:topic1,qos:exactly_once,retain:yes,dup:no,pid:4660,ps:[{id:content_type,val:json}]}"
    );
#endif // defined(ASYNC_MQTT_PRINT_PAYLOAD)

    auto p2 = am::v5::publish_packet{
        0x1234, // packet_id
        "topic1",
        "payload1",
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::no,
        am::properties{
            am::property::content_type("json")
        }
    };

    auto p3 = am::v5::publish_packet{
        0x1235, // packet_id
        "topic1",
        "payload1",
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::no,
        am::properties{
            am::property::content_type("json")
        }
    };
    BOOST_CHECK(p == p2);
    BOOST_CHECK(!(p < p2));
    BOOST_CHECK(!(p2< p));
    BOOST_CHECK(p < p3 || p3 < p);

    try {
        auto p = am::v5::publish_packet{
            0x1234, // packet_id
            "topic1",
            "payload1",
            am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
            am::properties{
                am::property::will_delay_interval{1}
            }
        };
        BOOST_TEST(false);
    }
    catch (am::system_error const& se) {
        BOOST_TEST(se.code() == am::disconnect_reason_code::malformed_packet);
    }

    try {
        auto p = am::v5::publish_packet{
            0x1234, // packet_id
            std::string(0x10000, 'w'), // too long
            "payload1",
            am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
            am::properties{
                am::property::content_type("json")
            }
        };
        BOOST_TEST(false);
    }
    catch (am::system_error const& se) {
        BOOST_TEST(se.code() == am::disconnect_reason_code::malformed_packet);
    }
}

BOOST_AUTO_TEST_CASE(v5_publish_qos0) {
    auto p = am::v5::publish_packet{
        "topic1",
        "payload1",
        am::qos::at_most_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{
            am::property::content_type("json")
        }
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
            0x18,                               // remaining_length
            0x00, 0x06,                         // topic_name_length
            0x74, 0x6f, 0x70, 0x69, 0x63, 0x31, // topic_name
            0x07,                               // property_length
            0x03,                               // content_type
            0x00, 0x04, 0x6a, 0x73, 0x6f, 0x6e, // json
            0x70, 0x61, 0x79, 0x6c, 0x6f, 0x61, 0x64, 0x31 // payload
        };
        auto [b, e] = am::make_packet_range(cbs);
        BOOST_TEST(std::equal(b, e, std::begin(expected)));

        am::buffer buf{std::begin(expected), std::end(expected)};
        am::error_code ec;
        auto p = am::v5::publish_packet{buf, ec};
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
    }
#if defined(ASYNC_MQTT_PRINT_PAYLOAD)
    BOOST_TEST(
        boost::lexical_cast<std::string>(p) ==
        "v5::publish{topic:topic1,qos:at_most_once,retain:yes,dup:no,payload:payload1,ps:[{id:content_type,val:json}]}"
    );
#else  // defined(ASYNC_MQTT_PRINT_PAYLOAD)
    BOOST_TEST(
        boost::lexical_cast<std::string>(p) ==
        "v5::publish{topic:topic1,qos:at_most_once,retain:yes,dup:no,ps:[{id:content_type,val:json}]}"
    );
#endif // defined(ASYNC_MQTT_PRINT_PAYLOAD)
}

BOOST_AUTO_TEST_CASE(v5_publish_invalid) {
    try {
        auto p = am::v5::publish_packet{
            "topic1",
            "payload1",
            am::qos::at_least_once | am::pub::retain::yes | am::pub::dup::yes
        };
        BOOST_TEST(false);
    }
    catch (am::system_error const& se) {
        BOOST_TEST(se.code() == am::disconnect_reason_code::protocol_error);
    }
    try {
        auto p = am::v5::publish_packet{
            1,
            "topic1",
            "payload1",
            am::qos::at_most_once | am::pub::retain::yes | am::pub::dup::yes
        };
        BOOST_TEST(false);
    }
    catch (am::system_error const& se) {
        BOOST_TEST(se.code() == am::disconnect_reason_code::protocol_error);
    }
}

BOOST_AUTO_TEST_CASE(v5_publish_pid4) {
    auto p = am::v5::basic_publish_packet<4>(
        0x12345678, // packet_id
        "topic1",
        "payload1",
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{
            am::property::content_type("json")
        }
    );

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
            0x1c,                               // remaining_length
            0x00, 0x06,                         // topic_name_length
            0x74, 0x6f, 0x70, 0x69, 0x63, 0x31, // topic_name
            0x12, 0x34, 0x56, 0x78,             // packet_id
            0x07,                               // property_length
            0x03,                               // content_type
            0x00, 0x04, 0x6a, 0x73, 0x6f, 0x6e, // json
            0x70, 0x61, 0x79, 0x6c, 0x6f, 0x61, 0x64, 0x31 // payload
        };
        auto [b, e] = am::make_packet_range(cbs);
        BOOST_TEST(std::equal(b, e, std::begin(expected)));

        am::buffer buf{std::begin(expected), std::end(expected)};
        am::error_code ec;
        auto p = am::v5::basic_publish_packet<4>{buf, ec};
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
    }
#if defined(ASYNC_MQTT_PRINT_PAYLOAD)
    BOOST_TEST(
        boost::lexical_cast<std::string>(p) ==
        "v5::publish{topic:topic1,qos:exactly_once,retain:yes,dup:no,pid:305419896,payload:payload1,ps:[{id:content_type,val:json}]}"
    );
#else  // defined(ASYNC_MQTT_PRINT_PAYLOAD)
    BOOST_TEST(
        boost::lexical_cast<std::string>(p) ==
        "v5::publish{topic:topic1,qos:exactly_once,retain:yes,dup:no,pid:305419896,ps:[{id:content_type,val:json}]}"
    );
#endif // defined(ASYNC_MQTT_PRINT_PAYLOAD)
}

BOOST_AUTO_TEST_CASE(v5_publish_topic_alias) {
    auto p1 = am::v5::publish_packet{
        "topic1",
        "payload1",
        am::qos::at_most_once | am::pub::retain::no | am::pub::dup::no,
        am::properties{
            am::property::topic_alias(1)
        }
    };
#if defined(ASYNC_MQTT_PRINT_PAYLOAD)
    BOOST_TEST(
        boost::lexical_cast<std::string>(p1) ==
        "v5::publish{topic:topic1,qos:at_most_once,retain:no,dup:no,payload:payload1,ps:[{id:topic_alias,val:1}]}"
    );
#else  // defined(ASYNC_MQTT_PRINT_PAYLOAD)
    BOOST_TEST(
        boost::lexical_cast<std::string>(p1) ==
        "v5::publish{topic:topic1,qos:at_most_once,retain:no,dup:no,ps:[{id:topic_alias,val:1}]}"
    );
#endif // defined(ASYNC_MQTT_PRINT_PAYLOAD)
    {
        auto cbs = p1.const_buffer_sequence();
        BOOST_TEST(cbs.size() == p1.num_of_const_buffer_sequence());
        char expected[] {
            0x30,                               // fixed_header
            0x14,                               // remaining_length
            0x00, 0x06,                         // topic_name_length
            0x74, 0x6f, 0x70, 0x69, 0x63, 0x31, // topic_name
            0x03,                               // property_length
            0x23,                               // topic_alias
            0x00, 0x01,                         // 1
            0x70, 0x61, 0x79, 0x6c, 0x6f, 0x61, 0x64, 0x31 // payload
        };
        auto [b, e] = am::make_packet_range(cbs);
        BOOST_TEST(std::equal(b, e, std::begin(expected)));
    }

    auto p2 = p1;
    p2.remove_topic_alias();
#if defined(ASYNC_MQTT_PRINT_PAYLOAD)
    BOOST_TEST(
        boost::lexical_cast<std::string>(p2) ==
        "v5::publish{topic:topic1,qos:at_most_once,retain:no,dup:no,payload:payload1}"
    );
#else  // defined(ASYNC_MQTT_PRINT_PAYLOAD)
    BOOST_TEST(
        boost::lexical_cast<std::string>(p2) ==
        "v5::publish{topic:topic1,qos:at_most_once,retain:no,dup:no}"
    );
#endif // defined(ASYNC_MQTT_PRINT_PAYLOAD)
    {
        auto cbs = p2.const_buffer_sequence();
        BOOST_TEST(cbs.size() == p2.num_of_const_buffer_sequence());
        char expected[] {
            0x30,                               // fixed_header
            0x11,                               // remaining_length
            0x00, 0x06,                         // topic_name_length
            0x74, 0x6f, 0x70, 0x69, 0x63, 0x31, // topic_name
            0x00,                               // property_length
            0x70, 0x61, 0x79, 0x6c, 0x6f, 0x61, 0x64, 0x31 // payload
        };
        auto [b, e] = am::make_packet_range(cbs);
        BOOST_TEST(std::equal(b, e, std::begin(expected)));
    }
    // p1 is not changed
    {
        auto cbs = p1.const_buffer_sequence();
        BOOST_TEST(cbs.size() == p1.num_of_const_buffer_sequence());
        char expected[] {
            0x30,                               // fixed_header
            0x14,                               // remaining_length
            0x00, 0x06,                         // topic_name_length
            0x74, 0x6f, 0x70, 0x69, 0x63, 0x31, // topic_name
            0x03,                               // property_length
            0x23,                               // topic_alias
            0x00, 0x01,                         // 1
            0x70, 0x61, 0x79, 0x6c, 0x6f, 0x61, 0x64, 0x31 // payload
        };
        auto [b, e] = am::make_packet_range(cbs);
        BOOST_TEST(std::equal(b, e, std::begin(expected)));
    }

    auto p3 = p2;
    p3.remove_topic_add_topic_alias(0x1234);
#if defined(ASYNC_MQTT_PRINT_PAYLOAD)
    BOOST_TEST(
        boost::lexical_cast<std::string>(p3) ==
        "v5::publish{topic:,qos:at_most_once,retain:no,dup:no,payload:payload1,ps:[{id:topic_alias,val:4660}]}"
    );
#else  // defined(ASYNC_MQTT_PRINT_PAYLOAD)
    BOOST_TEST(
        boost::lexical_cast<std::string>(p3) ==
        "v5::publish{topic:,qos:at_most_once,retain:no,dup:no,ps:[{id:topic_alias,val:4660}]}"
    );
#endif // defined(ASYNC_MQTT_PRINT_PAYLOAD)
    {
        auto cbs = p3.const_buffer_sequence();
        BOOST_TEST(cbs.size() == p3.num_of_const_buffer_sequence());
        char expected[] {
            0x30,                               // fixed_header
            0x0e,                               // remaining_length
            0x00, 0x00,                         // topic_name_length
            0x03,                               // property_length
            0x23,                               // topic_alias
            0x12, 0x34,                         // 0x1234
            0x70, 0x61, 0x79, 0x6c, 0x6f, 0x61, 0x64, 0x31 // payload
        };
        auto [b, e] = am::make_packet_range(cbs);
        BOOST_TEST(std::equal(b, e, std::begin(expected)));
    }
    // p2 is not changed
    {
        auto cbs = p2.const_buffer_sequence();
        BOOST_TEST(cbs.size() == p2.num_of_const_buffer_sequence());
        char expected[] {
            0x30,                               // fixed_header
            0x11,                               // remaining_length
            0x00, 0x06,                         // topic_name_length
            0x74, 0x6f, 0x70, 0x69, 0x63, 0x31, // topic_name
            0x00,                               // property_length
            0x70, 0x61, 0x79, 0x6c, 0x6f, 0x61, 0x64, 0x31 // payload
        };
        auto [b, e] = am::make_packet_range(cbs);
        BOOST_TEST(std::equal(b, e, std::begin(expected)));
    }

    auto p4 = p3;
    p4.remove_topic_alias_add_topic("topic1");
#if defined(ASYNC_MQTT_PRINT_PAYLOAD)
    BOOST_TEST(
        boost::lexical_cast<std::string>(p4) ==
        "v5::publish{topic:topic1,qos:at_most_once,retain:no,dup:no,payload:payload1}"
    );
#else  // defined(ASYNC_MQTT_PRINT_PAYLOAD)
    BOOST_TEST(
        boost::lexical_cast<std::string>(p4) ==
        "v5::publish{topic:topic1,qos:at_most_once,retain:no,dup:no}"
    );
#endif // defined(ASYNC_MQTT_PRINT_PAYLOAD)
    {
        auto cbs = p4.const_buffer_sequence();
        BOOST_TEST(cbs.size() == p4.num_of_const_buffer_sequence());
        char expected[] {
            0x30,                               // fixed_header
            0x11,                               // remaining_length
            0x00, 0x06,                         // topic_name_length
            0x74, 0x6f, 0x70, 0x69, 0x63, 0x31, // topic_name
            0x00,                               // property_length
            0x70, 0x61, 0x79, 0x6c, 0x6f, 0x61, 0x64, 0x31 // payload
        };
        auto [b, e] = am::make_packet_range(cbs);
        BOOST_TEST(std::equal(b, e, std::begin(expected)));
    }
    // p3 is not changed
    {
        auto cbs = p3.const_buffer_sequence();
        BOOST_TEST(cbs.size() == p3.num_of_const_buffer_sequence());
        char expected[] {
            0x30,                               // fixed_header
            0x0e,                               // remaining_length
            0x00, 0x00,                         // topic_name_length
            0x03,                               // property_length
            0x23,                               // topic_alias
            0x12, 0x34,                         // 0x1234
            0x70, 0x61, 0x79, 0x6c, 0x6f, 0x61, 0x64, 0x31 // payload
        };
        auto [b, e] = am::make_packet_range(cbs);
        BOOST_TEST(std::equal(b, e, std::begin(expected)));
    }

    auto p5 = p3;
    p5.add_topic("topic1");
#if defined(ASYNC_MQTT_PRINT_PAYLOAD)
    BOOST_TEST(
        boost::lexical_cast<std::string>(p5) ==
        "v5::publish{topic:topic1,qos:at_most_once,retain:no,dup:no,payload:payload1,ps:[{id:topic_alias,val:4660}]}"
    );
#else  // defined(ASYNC_MQTT_PRINT_PAYLOAD)
    BOOST_TEST(
        boost::lexical_cast<std::string>(p5) ==
        "v5::publish{topic:topic1,qos:at_most_once,retain:no,dup:no,ps:[{id:topic_alias,val:4660}]}"
    );
#endif // defined(ASYNC_MQTT_PRINT_PAYLOAD)
    {
        auto cbs = p5.const_buffer_sequence();
        BOOST_TEST(cbs.size() == p5.num_of_const_buffer_sequence());
        char expected[] {
            0x30,                               // fixed_header
            0x14,                               // remaining_length
            0x00, 0x06,                         // topic_name_length
            0x74, 0x6f, 0x70, 0x69, 0x63, 0x31, // topic_name
            0x03,                               // property_length
            0x23,                               // topic_alias
            0x12, 0x34,                         // 0x1234
            0x70, 0x61, 0x79, 0x6c, 0x6f, 0x61, 0x64, 0x31 // payload
        };
        auto [b, e] = am::make_packet_range(cbs);
        BOOST_TEST(std::equal(b, e, std::begin(expected)));
    }
    // p3 is not changed
    {
        auto cbs = p3.const_buffer_sequence();
        BOOST_TEST(cbs.size() == p3.num_of_const_buffer_sequence());
        char expected[] {
            0x30,                               // fixed_header
            0x0e,                               // remaining_length
            0x00, 0x00,                         // topic_name_length
            0x03,                               // property_length
            0x23,                               // topic_alias
            0x12, 0x34,                         // 0x1234
            0x70, 0x61, 0x79, 0x6c, 0x6f, 0x61, 0x64, 0x31 // payload
        };
        auto [b, e] = am::make_packet_range(cbs);
        BOOST_TEST(std::equal(b, e, std::begin(expected)));
    }
}

BOOST_AUTO_TEST_CASE(v5_publish_error) {
    {
        am::buffer buf; // empty
        am::error_code ec;
        am::v5::publish_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        //                CP
        am::buffer buf{"\x00"sv}; // invalid type
        am::error_code ec;
        am::v5::publish_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        //                CP
        am::buffer buf{"\x32"sv}; // short
        am::error_code ec;
        am::v5::publish_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        //                CP  RL
        am::buffer buf{"\x32\x01"sv}; // remaining length buf mismatch
        am::error_code ec;
        am::v5::publish_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        //                CP  RL
        am::buffer buf{"\x32\x03"sv}; // invalid remaining length
        am::error_code ec;
        am::v5::publish_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        //                CP  RL  TPL
        am::buffer buf{"\x32\x01\x00"sv}; // short topic name length
        am::error_code ec;
        am::v5::publish_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        //                CP  RL  TPL
        am::buffer buf{"\x32\x02\x02T"sv}; // mismatch topic name length
        am::error_code ec;
        am::v5::publish_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        //                CP  RL  TPL
        am::buffer buf{"\x32\x00\x00\x00"sv}; // zero remaining length
        am::error_code ec;
        am::v5::publish_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        //                CP  RL  TPL     PL
        am::buffer buf{"\x30\x03\x00\x00\x00"sv}; // no pid QoS0 valid
        am::error_code ec;
        am::v5::publish_packet{buf, ec};
        BOOST_TEST(ec == am::error_code{});
    }
    {
        //                CP  RL  TPL
        am::buffer buf{"\x36\x02\x00\x00"sv}; // QoS3 invalid
        am::error_code ec;
        am::v5::publish_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        //                CP  RL  TPL
        am::buffer buf{"\x32\x02\x00\x00"sv}; // no pid QoS1 invalid
        am::error_code ec;
        am::v5::publish_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        //                CP  RL  TPL   TP PID     PL
        am::buffer buf{"\x32\x06\x00\x01T\x12\x34\x00"sv}; // no payload (valid)
        am::error_code ec;
        am::v5::publish_packet{buf, ec};
        BOOST_TEST(ec == am::error_code{});
    }
    {
        //                CP  RL  TPL     TP      PID
        am::buffer buf{"\x32\x06\x00\x02\xc2\xc0\x12\x34"sv}; // invalid utf8 topic
        am::error_code ec;
        am::v5::publish_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        //                CP  RL  TPL   TP PID
        am::buffer buf{"\x32\x04\x00\x01T\x12"sv}; // invalid pid
        am::error_code ec;
        am::v5::publish_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        //                CP  RL  TPL   TP PID     PL
        am::buffer buf{"\x32\x05\x00\x01T\x12\x34\x01"sv}; // mismatch property length
        am::error_code ec;
        am::v5::publish_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        //                CP  RL  TPL   TP PID     PL
        am::buffer buf{"\x32\x06\x00\x01T\x12\x34\x01"sv}; // mismatch property length
        am::error_code ec;
        am::v5::publish_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        //                CP  RL  TPL   TP PID     PL
        am::buffer buf{"\x32\x06\x00\x01T\x12\x34\x80"sv}; // invalid property length
        am::error_code ec;
        am::v5::publish_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
}

BOOST_AUTO_TEST_SUITE_END()
