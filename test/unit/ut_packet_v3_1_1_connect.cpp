// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "../common/test_main.hpp"
#include "../common/global_fixture.hpp"

#include <boost/lexical_cast.hpp>

#include <async_mqtt/packet/v3_1_1_connect.hpp>
#include <async_mqtt/packet/packet_iterator.hpp>

BOOST_AUTO_TEST_SUITE(ut_packet)

namespace am = async_mqtt;

BOOST_AUTO_TEST_CASE(v311_connect) {
    auto w = am::will{
        am::allocate_buffer("topic1"),
        am::allocate_buffer("payload1"),
        am::pub::retain::yes | am::qos::at_least_once
    };

    auto p = am::v3_1_1::connect_packet{
        true,   // clean_session
        0x1234, // keep_alive
        am::allocate_buffer("cid1"),
        w,
        am::allocate_buffer("user1"),
        am::allocate_buffer("pass1")
    };

    BOOST_TEST(p.clean_session());
    BOOST_TEST(p.keep_alive() == 0x1234);
    BOOST_TEST(p.client_id() == "cid1");
    BOOST_TEST(p.get_will().has_value());
    BOOST_TEST(*p.get_will() == w);
    BOOST_TEST(p.user_name().has_value());
    BOOST_TEST(*p.user_name() == "user1");
    BOOST_TEST(p.password().has_value());
    BOOST_TEST(*p.password() == "pass1");

    {
        auto cbs = p.const_buffer_sequence();
        char expected[] {
            0x10,                               // fixed_header
            0x30,                               // remaining_length
            0x00, 0x04, 'M', 'Q', 'T', 'T',     // protocol_name
            0x04,                               // protocol_level
            char(0xee),                         // connect_flags
            0x12, 0x34,                         // keep_alive
            0x00, 0x04,                         // client_id_length
            0x63, 0x69, 0x64, 0x31,             // client_id
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
        auto p = am::v3_1_1::connect_packet(buf);
        BOOST_TEST(p.clean_session());
        BOOST_TEST(p.keep_alive() == 0x1234);
        BOOST_TEST(p.client_id() == "cid1");
        BOOST_TEST(p.get_will().has_value());
        BOOST_TEST(*p.get_will() == w);
        BOOST_TEST(p.user_name().has_value());
        BOOST_TEST(*p.user_name() == "user1");
        BOOST_TEST(p.password().has_value());
        BOOST_TEST(*p.password() == "pass1");

        auto cbs2 = p.const_buffer_sequence();
        auto [b2, e2] = am::make_packet_range(cbs2);
        BOOST_TEST(std::equal(b2, e2, std::begin(expected)));
    }
    BOOST_TEST(
        boost::lexical_cast<std::string>(p) ==
        "v3_1_1::connect{cid:cid1,ka:4660,cs:1,un:user1,pw:*****,will:{topic:topic1,message:payload1,qos:at_least_once,retain:yes}}"
    );
}

BOOST_AUTO_TEST_SUITE_END()
