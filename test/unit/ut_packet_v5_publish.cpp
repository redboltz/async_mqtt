// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "../common/test_main.hpp"
#include "../common/global_fixture.hpp"

#include <async_mqtt/packet/v5_publish.hpp>
#include <async_mqtt/packet/packet_iterator.hpp>
#include <async_mqtt/util/hex_dump.hpp>

BOOST_AUTO_TEST_SUITE(ut_packet)

namespace am = async_mqtt;

BOOST_AUTO_TEST_CASE(v5_publish) {
    auto p = am::v5::publish_packet(
        0x1234, // packet_id
        am::allocate_buffer("topic1"),
        am::allocate_buffer("payload1"),
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{
            am::property::content_type("json")
        }
    );
    std::cout << am::hex_dump(p) << std::endl;
    BOOST_TEST(p.packet_id() == 0x1234);
    BOOST_TEST(p.topic() == "topic1");
    {
        auto const& bs = p.payload();
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

        auto buf = am::allocate_buffer(std::begin(expected), std::end(expected));
        auto p = am::v5::publish_packet(buf);
        BOOST_TEST(p.packet_id() == 0x1234);
        BOOST_TEST(p.topic() == "topic1");
        {
            auto const& bs = p.payload();
            auto [b, e] = am::make_packet_range(bs);
            std::string_view expected = "payload1";
            BOOST_TEST(std::equal(b, e, expected.begin()));
        }
        BOOST_TEST(p.opts().get_qos() == am::qos::exactly_once);
        BOOST_TEST(p.opts().get_retain() == am::pub::retain::yes);
        BOOST_TEST(p.opts().get_dup() == am::pub::dup::no);
    }
}

BOOST_AUTO_TEST_CASE(v5_publish_pid4) {
    auto p = am::v5::basic_publish_packet<4>(
        0x12345678, // packet_id
        am::allocate_buffer("topic1"),
        am::allocate_buffer("payload1"),
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{
            am::property::content_type("json")
        }
    );

    BOOST_TEST(p.packet_id() == 0x12345678);
    BOOST_TEST(p.topic() == "topic1");
    {
        auto const& bs = p.payload();
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

        auto buf = am::allocate_buffer(std::begin(expected), std::end(expected));
        auto p = am::v5::basic_publish_packet<4>(buf);
        BOOST_TEST(p.packet_id() == 0x12345678);
        BOOST_TEST(p.topic() == "topic1");
        {
            auto const& bs = p.payload();
            auto [b, e] = am::make_packet_range(bs);
            std::string_view expected = "payload1";
            BOOST_TEST(std::equal(b, e, expected.begin()));
        }
        BOOST_TEST(p.opts().get_qos() == am::qos::exactly_once);
        BOOST_TEST(p.opts().get_retain() == am::pub::retain::yes);
        BOOST_TEST(p.opts().get_dup() == am::pub::dup::no);
    }
}

BOOST_AUTO_TEST_SUITE_END()
