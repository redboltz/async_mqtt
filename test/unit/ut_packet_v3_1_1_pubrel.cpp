// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "../common/test_main.hpp"
#include "../common/global_fixture.hpp"

#include <async_mqtt/packet/v3_1_1_pubrel.hpp>
#include <async_mqtt/packet/packet_iterator.hpp>

BOOST_AUTO_TEST_SUITE(ut_packet)

namespace am = async_mqtt;

BOOST_AUTO_TEST_CASE(v3_1_1_pubrel) {
    auto p = am::v3_1_1::pubrel_packet(
        0x1234 // packet_id
    );

    BOOST_TEST(p.packet_id() == 0x1234);

    {
        auto cbs = p.const_buffer_sequence();
        char expected[] {
            0x62,                               // fixed_header
            0x02,                               // remaining_length
            0x12, 0x34                          // packet_id
        };
        auto [b, e] = am::make_packet_range(cbs);
        BOOST_TEST(std::equal(b, e, std::begin(expected)));

        auto buf = am::allocate_buffer(std::begin(expected), std::end(expected));
        auto p = am::v3_1_1::pubrel_packet(buf);
        BOOST_TEST(p.packet_id() == 0x1234);
    }
}

BOOST_AUTO_TEST_SUITE_END()
