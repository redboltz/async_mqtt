// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "../common/test_main.hpp"
#include "../common/global_fixture.hpp"

#include <async_mqtt/packet/v3_1_1_suback.hpp>
#include <async_mqtt/packet/packet_iterator.hpp>

BOOST_AUTO_TEST_SUITE(ut_packet)

namespace am = async_mqtt;

BOOST_AUTO_TEST_CASE(v3_1_1_suback) {
    std::vector<am::suback_return_code> args {
        am::suback_return_code::success_maximum_qos_1,
        am::suback_return_code::failure
    };
    auto p = am::v3_1_1::suback_packet{
        0x1234,         // packet_id
        args
    };

    BOOST_TEST(p.packet_id() == 0x1234);
    BOOST_TEST((p.entries() == args));
    {
        auto cbs = p.const_buffer_sequence();
        char expected[] {
            char(0x90),                                     // fixed_header
            0x04,                                           // remaining_length
            0x12, 0x34,                                     // packet_id
            0x01,                                           // suback_return_code1
            char(0x80)                                      // suback_return_code2
        };
        auto [b, e] = am::make_packet_range(cbs);
        BOOST_TEST(std::equal(b, e, std::begin(expected)));

        auto buf = am::allocate_buffer(std::begin(expected), std::end(expected));
        auto p = am::v3_1_1::suback_packet(buf);
        BOOST_TEST(p.packet_id() == 0x1234);
        BOOST_TEST((p.entries() == args));

        auto cbs2 = p.const_buffer_sequence();
        auto [b2, e2] = am::make_packet_range(cbs2);
        BOOST_TEST(std::equal(b2, e2, std::begin(expected)));
    }
}

BOOST_AUTO_TEST_CASE(v3_1_1_suback_pid4) {
    std::vector<am::suback_return_code> args {
        am::suback_return_code::success_maximum_qos_1,
        am::suback_return_code::failure
    };
    auto p = am::v3_1_1::basic_suback_packet<4>{
        0x12345678,         // packet_id
        args
    };

    BOOST_TEST(p.packet_id() == 0x12345678);
    BOOST_TEST((p.entries() == args));
    {
        auto cbs = p.const_buffer_sequence();
        char expected[] {
            char(0x90),                                     // fixed_header
            0x06,                                           // remaining_length
            0x12, 0x34, 0x56, 0x78,                         // packet_id
            0x01,                                           // suback_return_code1
            char(0x80)                                      // suback_return_code2
        };
        auto [b, e] = am::make_packet_range(cbs);
        BOOST_TEST(std::equal(b, e, std::begin(expected)));

        auto buf = am::allocate_buffer(std::begin(expected), std::end(expected));
        auto p = am::v3_1_1::basic_suback_packet<4>(buf);
        BOOST_TEST(p.packet_id() == 0x12345678);
        BOOST_TEST((p.entries() == args));

        auto cbs2 = p.const_buffer_sequence();
        auto [b2, e2] = am::make_packet_range(cbs2);
        BOOST_TEST(std::equal(b2, e2, std::begin(expected)));
    }
}

BOOST_AUTO_TEST_SUITE_END()
