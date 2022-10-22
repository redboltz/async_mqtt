// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "../common/test_main.hpp"
#include "../common/global_fixture.hpp"

#include <boost/lexical_cast.hpp>

#include <async_mqtt/packet/v5_subscribe.hpp>
#include <async_mqtt/packet/packet_iterator.hpp>

BOOST_AUTO_TEST_SUITE(ut_packet)

namespace am = async_mqtt;

BOOST_AUTO_TEST_CASE(v5_subscribe) {
    std::vector<am::topic_subopts> args {
        {am::allocate_buffer("topic1"), am::qos::at_most_once | am::sub::nl::yes | am::sub::retain_handling::not_send},
        {am::allocate_buffer("topic2"), am::qos::exactly_once | am::sub::rap::retain},
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

        auto buf = am::allocate_buffer(std::begin(expected), std::end(expected));
        auto p = am::v5::subscribe_packet(buf);
        BOOST_TEST(p.packet_id() == 0x1234);
        BOOST_TEST((p.entries() == args));
        BOOST_TEST(p.props() == props);

        auto cbs2 = p.const_buffer_sequence();
        auto [b2, e2] = am::make_packet_range(cbs2);
        BOOST_TEST(std::equal(b2, e2, std::begin(expected)));
    }
    BOOST_TEST(
        boost::lexical_cast<std::string>(p) ==
        "v5::subscribe{pid:4660,[{topic:topic1,sn:,qos:at_most_once,rh:not_send,nl:yes,rap:dont},{topic:topic2,sn:,qos:exactly_once,rh:send,nl:no,rap:retain}],ps:[{id:subscription_identifier,val:268435455}]}"
    );
}

BOOST_AUTO_TEST_CASE(v5_subscribe_pid4) {
    std::vector<am::topic_subopts> args {
        {am::allocate_buffer("topic1"), am::qos::at_most_once | am::sub::nl::yes | am::sub::retain_handling::not_send},
        {am::allocate_buffer("topic2"), am::qos::exactly_once | am::sub::rap::retain},
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

        auto buf = am::allocate_buffer(std::begin(expected), std::end(expected));
        auto p = am::v5::basic_subscribe_packet<4>(buf);
        BOOST_TEST(p.packet_id() == 0x12345678);
        BOOST_TEST((p.entries() == args));

        auto cbs2 = p.const_buffer_sequence();
        auto [b2, e2] = am::make_packet_range(cbs2);
        BOOST_TEST(std::equal(b2, e2, std::begin(expected)));
    }
    BOOST_TEST(
        boost::lexical_cast<std::string>(p) ==
        "v5::subscribe{pid:305419896,[{topic:topic1,sn:,qos:at_most_once,rh:not_send,nl:yes,rap:dont},{topic:topic2,sn:,qos:exactly_once,rh:send,nl:no,rap:retain}],ps:[{id:subscription_identifier,val:268435455}]}"
    );
}

BOOST_AUTO_TEST_SUITE_END()
