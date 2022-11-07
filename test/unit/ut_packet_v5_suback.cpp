// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "../common/test_main.hpp"
#include "../common/global_fixture.hpp"

#include <async_mqtt/packet/v5_suback.hpp>
#include <async_mqtt/packet/packet_iterator.hpp>

BOOST_AUTO_TEST_SUITE(ut_packet)

namespace am = async_mqtt;

BOOST_AUTO_TEST_CASE(v5_suback) {
    std::vector<am::suback_reason_code> args {
        am::suback_reason_code::granted_qos_1,
        am::suback_reason_code::unspecified_error
    };
    auto props = am::properties{
        am::property::reason_string("some reason")
    };
    auto p = am::v5::suback_packet{
        0x1234,         // packet_id
        args,
        props
    };

    BOOST_TEST(p.packet_id() == 0x1234);
    BOOST_TEST((p.entries() == args));
    {
        auto cbs = p.const_buffer_sequence();
        char expected[] {
            char(0x90),                                     // fixed_header
            0x13,                                           // remaining_length
            0x12, 0x34,                                     // packet_id
            0x0e,                                           // property_length
            0x1f,                                           // reason_string
            0x00, 0x0b, 0x73, 0x6f, 0x6d, 0x65, 0x20, 0x72, 0x65, 0x61, 0x73, 0x6f, 0x6e,
            0x01,                                           // suback_reason_code1
            char(0x80)                                      // suback_reason_code2
        };
        auto [b, e] = am::make_packet_range(cbs);
        BOOST_TEST(std::equal(b, e, std::begin(expected)));

        auto buf = am::allocate_buffer(std::begin(expected), std::end(expected));
        auto p = am::v5::suback_packet(buf);
        BOOST_TEST(p.packet_id() == 0x1234);
        BOOST_TEST((p.entries() == args));

        auto cbs2 = p.const_buffer_sequence();
        auto [b2, e2] = am::make_packet_range(cbs2);
        BOOST_TEST(std::equal(b2, e2, std::begin(expected)));
    }
}

BOOST_AUTO_TEST_CASE(v5_suback_pid4) {
    std::vector<am::suback_reason_code> args {
        am::suback_reason_code::granted_qos_1,
        am::suback_reason_code::unspecified_error
    };
    auto props = am::properties{
        am::property::reason_string("some reason")
    };
    auto p = am::v5::basic_suback_packet<4>{
        0x12345678,         // packet_id
        args,
        props
    };

    BOOST_TEST(p.packet_id() == 0x12345678);
    BOOST_TEST((p.entries() == args));
    {
        auto cbs = p.const_buffer_sequence();
        char expected[] {
            char(0x90),                                     // fixed_header
            0x15,                                           // remaining_length
            0x12, 0x34, 0x56, 0x78,                         // packet_id
            0x0e,                                           // property_length
            0x1f,                                           // reason_string
            0x00, 0x0b, 0x73, 0x6f, 0x6d, 0x65, 0x20, 0x72, 0x65, 0x61, 0x73, 0x6f, 0x6e,
            0x01,                                           // suback_reason_code1
            char(0x80)                                      // suback_reason_code2
        };
        auto [b, e] = am::make_packet_range(cbs);
        BOOST_TEST(std::equal(b, e, std::begin(expected)));

        auto buf = am::allocate_buffer(std::begin(expected), std::end(expected));
        auto p = am::v5::basic_suback_packet<4>(buf);
        BOOST_TEST(p.packet_id() == 0x12345678);
        BOOST_TEST((p.entries() == args));

        auto cbs2 = p.const_buffer_sequence();
        auto [b2, e2] = am::make_packet_range(cbs2);
        BOOST_TEST(std::equal(b2, e2, std::begin(expected)));
    }
}

BOOST_AUTO_TEST_SUITE_END()
