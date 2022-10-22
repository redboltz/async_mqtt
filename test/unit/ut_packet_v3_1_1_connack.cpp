// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "../common/test_main.hpp"
#include "../common/global_fixture.hpp"

#include <boost/lexical_cast.hpp>

#include <async_mqtt/packet/v3_1_1_connack.hpp>
#include <async_mqtt/packet/packet_iterator.hpp>

BOOST_AUTO_TEST_SUITE(ut_packet)

namespace am = async_mqtt;

BOOST_AUTO_TEST_CASE(v311_connack) {
    auto p = am::v3_1_1::connack_packet{
        true,   // session_present
        am::connect_return_code::not_authorized
    };

    BOOST_TEST(p.session_present());
    BOOST_TEST(p.code() == am::connect_return_code::not_authorized);

    {
        auto cbs = p.const_buffer_sequence();
        char expected[] {
            0x20, // fixed_header
            0x02, // remaining_length
            0x01, // session_present
            0x05, // connect_return_code
                };
        auto [b, e] = am::make_packet_range(cbs);
        BOOST_TEST(std::equal(b, e, std::begin(expected)));

        auto buf = am::allocate_buffer(std::begin(expected), std::end(expected));
        auto p = am::v3_1_1::connack_packet(buf);
        BOOST_TEST(p.session_present());
        BOOST_TEST(p.code() == am::connect_return_code::not_authorized);

        auto cbs2 = p.const_buffer_sequence();
        auto [b2, e2] = am::make_packet_range(cbs2);
        BOOST_TEST(std::equal(b2, e2, std::begin(expected)));
    }
    BOOST_TEST(
        boost::lexical_cast<std::string>(p) ==
        "v3_1_1::connack{rc:not_authorized,sp:1}"
    );
}

BOOST_AUTO_TEST_SUITE_END()
