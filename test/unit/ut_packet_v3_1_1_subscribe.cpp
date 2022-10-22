// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "../common/test_main.hpp"
#include "../common/global_fixture.hpp"

#include <boost/lexical_cast.hpp>

#include <async_mqtt/packet/v3_1_1_subscribe.hpp>
#include <async_mqtt/packet/packet_iterator.hpp>

BOOST_AUTO_TEST_SUITE(ut_packet)

namespace am = async_mqtt;

BOOST_AUTO_TEST_CASE(v311_subscribe) {
    std::vector<am::topic_subopts> args {
        {am::allocate_buffer("topic1"), am::qos::at_most_once},
        {am::allocate_buffer("topic2"), am::qos::exactly_once},
    };

    auto p = am::v3_1_1::subscribe_packet{
        0x1234,         // packet_id
        args
    };

    BOOST_TEST(p.packet_id() == 0x1234);
    BOOST_TEST((p.entries() == args));
    {
        auto cbs = p.const_buffer_sequence();
        char expected[] {
            char(0x82),                                     // fixed_header
            0x14,                                           // remaining_length
            0x12, 0x34,                                     // packet_id
            0x00, 0x06, 0x74, 0x6f, 0x70, 0x69, 0x63, 0x31, // topic1
            0x00,                                           // opts1
            0x00, 0x06, 0x74, 0x6f, 0x70, 0x69, 0x63, 0x32, // topic2
            0x02                                            // opts2
        };
        auto [b, e] = am::make_packet_range(cbs);
        BOOST_TEST(std::equal(b, e, std::begin(expected)));

        auto buf = am::allocate_buffer(std::begin(expected), std::end(expected));
        auto p = am::v3_1_1::subscribe_packet(buf);
        BOOST_TEST(p.packet_id() == 0x1234);
        BOOST_TEST((p.entries() == args));

        auto cbs2 = p.const_buffer_sequence();
        auto [b2, e2] = am::make_packet_range(cbs2);
        BOOST_TEST(std::equal(b2, e2, std::begin(expected)));
    }
    BOOST_TEST(
        boost::lexical_cast<std::string>(p) ==
        "v3_1_1::subscribe{pid:4660,[{topic:topic1,qos:at_most_once},{topic:topic2,qos:exactly_once}]}"
    );
}

BOOST_AUTO_TEST_CASE(v311_subscribe_pid4) {
    std::vector<am::topic_subopts> args {
        {am::allocate_buffer("topic1"), am::qos::at_most_once},
        {am::allocate_buffer("topic2"), am::qos::exactly_once},
    };

    auto p = am::v3_1_1::basic_subscribe_packet<4>{
        0x12345678,         // packet_id
        args
    };

    BOOST_TEST(p.packet_id() == 0x12345678);
    BOOST_TEST((p.entries() == args));
    {
        auto cbs = p.const_buffer_sequence();
        char expected[] {
            char(0x82),                                     // fixed_header
            0x16,                                           // remaining_length
            0x12, 0x34, 0x56, 0x78,                         // packet_id
            0x00, 0x06, 0x74, 0x6f, 0x70, 0x69, 0x63, 0x31, // topic1
            0x00,                                           // opts1
            0x00, 0x06, 0x74, 0x6f, 0x70, 0x69, 0x63, 0x32, // topic2
            0x02                                            // opts2
        };
        auto [b, e] = am::make_packet_range(cbs);
        BOOST_TEST(std::equal(b, e, std::begin(expected)));

        auto buf = am::allocate_buffer(std::begin(expected), std::end(expected));
        auto p = am::v3_1_1::basic_subscribe_packet<4>(buf);
        BOOST_TEST(p.packet_id() == 0x12345678);
        BOOST_TEST((p.entries() == args));

        auto cbs2 = p.const_buffer_sequence();
        auto [b2, e2] = am::make_packet_range(cbs2);
        BOOST_TEST(std::equal(b2, e2, std::begin(expected)));
    }
    BOOST_TEST(
        boost::lexical_cast<std::string>(p) ==
        "v3_1_1::subscribe{pid:305419896,[{topic:topic1,qos:at_most_once},{topic:topic2,qos:exactly_once}]}"
    );
}

BOOST_AUTO_TEST_SUITE_END()
