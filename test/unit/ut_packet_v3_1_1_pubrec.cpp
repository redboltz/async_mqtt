// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "../common/test_main.hpp"
#include "../common/global_fixture.hpp"

#include <boost/lexical_cast.hpp>

#define ASYNC_MQTT_UNIT_TEST_FOR_PACKET

BOOST_AUTO_TEST_SUITE(ut_packet)
struct v311_pubrec;
struct v311_pubrec_pid4;
struct v311_pubrec_error;
BOOST_AUTO_TEST_SUITE_END()

#include <async_mqtt/protocol/packet/v3_1_1_pubrec.hpp>
#include <async_mqtt/protocol/packet/packet_iterator.hpp>
#include <async_mqtt/protocol/packet/packet_traits.hpp>

BOOST_AUTO_TEST_SUITE(ut_packet)

namespace am = async_mqtt;
using namespace std::literals::string_view_literals;

BOOST_AUTO_TEST_CASE(v311_pubrec) {
    BOOST_TEST(am::is_pubrec<am::v3_1_1::pubrec_packet>());
    BOOST_TEST(am::is_v3_1_1<am::v3_1_1::pubrec_packet>());
    BOOST_TEST(!am::is_v5<am::v3_1_1::pubrec_packet>());
    BOOST_TEST(am::is_client_sendable<am::v3_1_1::pubrec_packet>());
    BOOST_TEST(am::is_server_sendable<am::v3_1_1::pubrec_packet>());

    auto p = am::v3_1_1::pubrec_packet{
        0x1234 // packet_id
    };

    BOOST_TEST(p.packet_id() == 0x1234);

    {
        auto cbs = p.const_buffer_sequence();
        BOOST_TEST(cbs.size() == p.num_of_const_buffer_sequence());
        char expected[] {
            0x50,                               // fixed_header
            0x02,                               // remaining_length
            0x12, 0x34                          // packet_id
        };
        auto [b, e] = am::make_packet_range(cbs);
        BOOST_TEST(std::equal(b, e, std::begin(expected)));

        am::buffer buf{std::begin(expected), std::end(expected)};
        am::error_code ec;
        auto p = am::v3_1_1::pubrec_packet{buf, ec};
        BOOST_TEST(!ec);
        BOOST_TEST(p.packet_id() == 0x1234);

        auto cbs2 = p.const_buffer_sequence();
        BOOST_TEST(cbs2.size() == p.num_of_const_buffer_sequence());
        auto [b2, e2] = am::make_packet_range(cbs2);
        BOOST_TEST(std::equal(b2, e2, std::begin(expected)));

        BOOST_TEST(p.type() == am::control_packet_type::pubrec);
    }
    BOOST_TEST(
        boost::lexical_cast<std::string>(p) ==
        "v3_1_1::pubrec{pid:4660}"
    );

    auto p2 = am::v3_1_1::pubrec_packet{
        0x1234 // packet_id
    };
    auto p3 = am::v3_1_1::pubrec_packet{
        0x1235 // packet_id
    };
    BOOST_CHECK(p == p2);
    BOOST_CHECK(!(p < p2));
    BOOST_CHECK(!(p2< p));
    BOOST_CHECK(p < p3 || p3 < p);
}

BOOST_AUTO_TEST_CASE(v311_pubrec_pid4) {
    auto p = am::v3_1_1::basic_pubrec_packet<4>{
        0x12345678 // packet_id
    };

    BOOST_TEST(p.packet_id() == 0x12345678);

    {
        auto cbs = p.const_buffer_sequence();
        BOOST_TEST(cbs.size() == p.num_of_const_buffer_sequence());
        char expected[] {
            0x50,                               // fixed_header
            0x04,                               // remaining_length
            0x12, 0x34, 0x56, 0x78              // packet_id
        };
        auto [b, e] = am::make_packet_range(cbs);
        BOOST_TEST(std::equal(b, e, std::begin(expected)));

        am::buffer buf{std::begin(expected), std::end(expected)};
        am::error_code ec;
        auto p = am::v3_1_1::basic_pubrec_packet<4>{buf, ec};
        BOOST_TEST(!ec);
        BOOST_TEST(p.packet_id() == 0x12345678);

        auto cbs2 = p.const_buffer_sequence();
        BOOST_TEST(cbs2.size() == p.num_of_const_buffer_sequence());
        auto [b2, e2] = am::make_packet_range(cbs2);
        BOOST_TEST(std::equal(b2, e2, std::begin(expected)));
    }
    BOOST_TEST(
        boost::lexical_cast<std::string>(p) ==
        "v3_1_1::pubrec{pid:305419896}"
    );
}

BOOST_AUTO_TEST_CASE(v311_pubrec_error) {
    {
        am::buffer buf; // empty
        am::error_code ec;
        am::v3_1_1::pubrec_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        //                CP
        am::buffer buf{"\x00"sv}; // invalid type
        am::error_code ec;
        am::v3_1_1::pubrec_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        //                CP
        am::buffer buf{"\x50"sv}; // short
        am::error_code ec;
        am::v3_1_1::pubrec_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        //                CP  RL
        am::buffer buf{"\x50\x01"sv}; // remaining length buf mismatch
        am::error_code ec;
        am::v3_1_1::pubrec_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        //                CP  RL
        am::buffer buf{"\x50\x03"sv}; // invalid remaining length
        am::error_code ec;
        am::v3_1_1::pubrec_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        //                CP  RL  PID
        am::buffer buf{"\x50\x02\x12\x34"sv}; // valid
        am::error_code ec;
        am::v3_1_1::pubrec_packet{buf, ec};
        BOOST_TEST(ec == am::errc::success);
    }
    {
        //                CP  RL  PID
        am::buffer buf{"\x50\x02\x12\x34\x56"sv}; // too long
        am::error_code ec;
        am::v3_1_1::pubrec_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
}

BOOST_AUTO_TEST_SUITE_END()
