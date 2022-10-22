// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "../common/test_main.hpp"
#include "../common/global_fixture.hpp"

#include <boost/lexical_cast.hpp>

#include <async_mqtt/packet/v5_connack.hpp>
#include <async_mqtt/packet/packet_iterator.hpp>

BOOST_AUTO_TEST_SUITE(ut_packet)

namespace am = async_mqtt;

BOOST_AUTO_TEST_CASE(v5_connack) {
    auto props = am::properties{
        am::property::session_expiry_interval(0x0fffffff)
    };
    auto p = am::v5::connack_packet{
        true,   // session_present
        am::connect_reason_code::not_authorized,
        props
    };
    BOOST_TEST(p.session_present());
    BOOST_TEST(p.code() == am::connect_reason_code::not_authorized);
    BOOST_TEST(p.props() == props);

    {
        auto cbs = p.const_buffer_sequence();
        char expected[] {
            0x20,       // fixed_header
            0x08,       // remaining_length
            0x01,       // session_present
            char(0x87), // connect_reason_code
            0x05,       // property_length
            0x11, 0x0f, char(0xff), char(0xff), char(0xff), // session_expiry_interval
        };
        auto [b, e] = am::make_packet_range(cbs);
        BOOST_TEST(std::equal(b, e, std::begin(expected)));

        auto buf = am::allocate_buffer(std::begin(expected), std::end(expected));
        auto p = am::v5::connack_packet(buf);
        BOOST_TEST(p.session_present());
        BOOST_TEST(p.code() == am::connect_reason_code::not_authorized);
        BOOST_TEST(p.props() == props);

        auto cbs2 = p.const_buffer_sequence();
        auto [b2, e2] = am::make_packet_range(cbs2);
        BOOST_TEST(std::equal(b2, e2, std::begin(expected)));
    }
    BOOST_TEST(
        boost::lexical_cast<std::string>(p) ==
        "v5::connack{rc:not_authorized,sp:1,ps:[{id:session_expiry_interval,val:268435455}]}"
    );
}

BOOST_AUTO_TEST_SUITE_END()
