// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "../common/test_main.hpp"
#include "../common/global_fixture.hpp"

#include <boost/lexical_cast.hpp>

#include <async_mqtt/packet/v5_connect.hpp>
#include <async_mqtt/packet/packet_iterator.hpp>

BOOST_AUTO_TEST_SUITE(ut_packet)

namespace am = async_mqtt;

BOOST_AUTO_TEST_CASE(v5_connect) {
    auto w = am::will{
        am::allocate_buffer("topic1"),
        am::allocate_buffer("payload1"),
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
        am::allocate_buffer("cid1"),
        w,
        am::allocate_buffer("user1"),
        am::allocate_buffer("pass1"),
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

        auto buf = am::allocate_buffer(std::begin(expected), std::end(expected));
        auto p = am::v5::connect_packet(buf);
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
        auto [b2, e2] = am::make_packet_range(cbs2);
        BOOST_TEST(std::equal(b2, e2, std::begin(expected)));
    }
    BOOST_TEST(
        boost::lexical_cast<std::string>(p) ==
        "v5::connect{cid:cid1,ka:4660,cs:1,un:user1,pw:*****,will:{topic:topic1,message:payload1,qos:at_least_once,retain:yes,ps:[{id:will_delay_interval,val:268435455},{id:content_type,val:json}]},ps:[{id:session_expiry_interval,val:268435455},{id:user_property,key:mykey,val:myval}]}"
    );
}

BOOST_AUTO_TEST_SUITE_END()
