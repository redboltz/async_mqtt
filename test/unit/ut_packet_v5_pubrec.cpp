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
struct v5_pubrec;
struct v5_pubrec_pid4;
struct v5_pubrec_pid_only;
struct v5_pubrec_pid_rc;
struct v5_pubrec_prop_len_last;
struct v5_pubrec_error;
BOOST_AUTO_TEST_SUITE_END()

#include <async_mqtt/packet/v5_pubrec.hpp>
#include <async_mqtt/packet/packet_iterator.hpp>
#include <async_mqtt/packet/packet_traits.hpp>

BOOST_AUTO_TEST_SUITE(ut_packet)

namespace am = async_mqtt;
using namespace std::literals::string_view_literals;

BOOST_AUTO_TEST_CASE(v5_pubrec) {
    BOOST_TEST(am::is_pubrec<am::v5::pubrec_packet>());
    BOOST_TEST(!am::is_v3_1_1<am::v5::pubrec_packet>());
    BOOST_TEST(am::is_v5<am::v5::pubrec_packet>());
    BOOST_TEST(am::is_client_sendable<am::v5::pubrec_packet>());
    BOOST_TEST(am::is_server_sendable<am::v5::pubrec_packet>());

    auto props = am::properties{
        am::property::reason_string("some reason")
    };
    auto p = am::v5::pubrec_packet{
        0x1234, // packet_id
        am::pubrec_reason_code::packet_identifier_in_use,
        props
    };
    BOOST_TEST(p.packet_id() == 0x1234);
    BOOST_TEST(p.code() == am::pubrec_reason_code::packet_identifier_in_use);
    BOOST_TEST(p.props() == props);

    {
        auto cbs = p.const_buffer_sequence();
        BOOST_TEST(cbs.size() == p.num_of_const_buffer_sequence());
        char expected[] {
            0x50,                               // fixed_header
            0x12,                               // remaining_length
            0x12, 0x34,                         // packet_id
            char(0x91),                         // reason_code
            0x0e,                               // property_length
            0x1f,                               // reason_string
            0x00, 0x0b, 0x73, 0x6f, 0x6d, 0x65, 0x20, 0x72, 0x65, 0x61, 0x73, 0x6f, 0x6e,
        };
        auto [b, e] = am::make_packet_range(cbs);
        BOOST_TEST(std::equal(b, e, std::begin(expected)));

        am::buffer buf{std::begin(expected), std::end(expected)};
        am::error_code ec;
        auto p = am::v5::pubrec_packet{buf, ec};
        BOOST_TEST(!ec);
        BOOST_TEST(p.packet_id() == 0x1234);
        BOOST_TEST(p.code() == am::pubrec_reason_code::packet_identifier_in_use);
        BOOST_TEST(p.props() == props);

        auto cbs2 = p.const_buffer_sequence();
        BOOST_TEST(cbs2.size() == p.num_of_const_buffer_sequence());
        auto [b2, e2] = am::make_packet_range(cbs2);
        BOOST_TEST(std::equal(b2, e2, std::begin(expected)));

        BOOST_TEST(p.type() == am::control_packet_type::pubrec);
    }
    BOOST_TEST(
        boost::lexical_cast<std::string>(p) ==
        "v5::pubrec{pid:4660,rc:packet_identifier_in_use,ps:[{id:reason_string,val:some reason}]}"
    );

    auto p2 = am::v5::pubrec_packet{
        0x1234, // packet_id
        am::pubrec_reason_code::packet_identifier_in_use,
        props
    };
    auto p3 = am::v5::pubrec_packet{
        0x1235, // packet_id
        am::pubrec_reason_code::packet_identifier_in_use,
        props
    };
    BOOST_CHECK(p == p2);
    BOOST_CHECK(!(p < p2));
    BOOST_CHECK(!(p2< p));
    BOOST_CHECK(p < p3 || p3 < p);

    try {
        auto p = am::v5::pubrec_packet{
            0x1234, // packet_id
            am::pubrec_reason_code::packet_identifier_in_use,
            am::properties{
                am::property::will_delay_interval{1}
            }
        };
        BOOST_TEST(false);
    }
    catch (am::system_error const& se) {
        BOOST_TEST(se.code() == am::disconnect_reason_code::malformed_packet);
    }
}

BOOST_AUTO_TEST_CASE(v5_pubrec_pid4) {
    auto props = am::properties{
        am::property::reason_string("some reason")
    };
    auto p = am::v5::basic_pubrec_packet<4>{
        0x12345678, // packet_id
        am::pubrec_reason_code::packet_identifier_in_use,
        props
    };

    BOOST_TEST(p.packet_id() == 0x12345678);
    BOOST_TEST(p.code() == am::pubrec_reason_code::packet_identifier_in_use);
    BOOST_TEST(p.props() == props);

    {
        auto cbs = p.const_buffer_sequence();
        BOOST_TEST(cbs.size() == p.num_of_const_buffer_sequence());
        char expected[] {
            0x50,                               // fixed_header
            0x14,                               // remaining_length
            0x12, 0x34, 0x56, 0x78,             // packet_id
            char(0x91),                         // reason_code
            0x0e,                               // property_length
            0x1f,                               // reason_string
            0x00, 0x0b, 0x73, 0x6f, 0x6d, 0x65, 0x20, 0x72, 0x65, 0x61, 0x73, 0x6f, 0x6e,
        };
        auto [b, e] = am::make_packet_range(cbs);
        BOOST_TEST(std::equal(b, e, std::begin(expected)));

        am::buffer buf{std::begin(expected), std::end(expected)};
        am::error_code ec;
        auto p = am::v5::basic_pubrec_packet<4>{buf, ec};
        BOOST_TEST(!ec);
        BOOST_TEST(p.packet_id() == 0x12345678);
        BOOST_TEST(p.code() == am::pubrec_reason_code::packet_identifier_in_use);
        BOOST_TEST(p.props() == props);

        auto cbs2 = p.const_buffer_sequence();
        BOOST_TEST(cbs2.size() == p.num_of_const_buffer_sequence());
        auto [b2, e2] = am::make_packet_range(cbs2);
        BOOST_TEST(std::equal(b2, e2, std::begin(expected)));
    }
    BOOST_TEST(
        boost::lexical_cast<std::string>(p) ==
        "v5::pubrec{pid:305419896,rc:packet_identifier_in_use,ps:[{id:reason_string,val:some reason}]}"
    );
}

BOOST_AUTO_TEST_CASE(v5_pubrec_pid_only) {
    auto p = am::v5::pubrec_packet{
        0x1234 // packet_id
    };
    BOOST_TEST(p.code() == am::pubrec_reason_code::success);
    BOOST_TEST(p.props().empty());
    BOOST_TEST(p.packet_id() == 0x1234);

    {
        auto cbs = p.const_buffer_sequence();
        BOOST_TEST(cbs.size() == p.num_of_const_buffer_sequence());
        char expected[] {
            0x50,                               // fixed_header
            0x02,                               // remaining_length
            0x12, 0x34,                         // packet_id
        };
        auto [b, e] = am::make_packet_range(cbs);
        BOOST_TEST(std::equal(b, e, std::begin(expected)));

        am::buffer buf{std::begin(expected), std::end(expected)};
        am::error_code ec;
        auto p = am::v5::pubrec_packet{buf, ec};
        BOOST_TEST(!ec);
        BOOST_TEST(p.packet_id() == 0x1234);
        BOOST_TEST(p.code() == am::pubrec_reason_code::success);
        BOOST_TEST(p.props().empty());

        auto cbs2 = p.const_buffer_sequence();
        BOOST_TEST(cbs2.size() == p.num_of_const_buffer_sequence());
        auto [b2, e2] = am::make_packet_range(cbs2);
        BOOST_TEST(std::equal(b2, e2, std::begin(expected)));
    }
    BOOST_TEST(
        boost::lexical_cast<std::string>(p) ==
        "v5::pubrec{pid:4660}"
    );
}

BOOST_AUTO_TEST_CASE(v5_pubrec_pid_rc) {
    auto p = am::v5::pubrec_packet{
        0x1234, // packet_id
        am::pubrec_reason_code::success
    };
    BOOST_TEST(p.code() == am::pubrec_reason_code::success);
    BOOST_TEST(p.props().empty());
    BOOST_TEST(p.packet_id() == 0x1234);

    {
        auto cbs = p.const_buffer_sequence();
        BOOST_TEST(cbs.size() == p.num_of_const_buffer_sequence());
        char expected[] {
            0x50,                               // fixed_header
            0x03,                               // remaining_length
            0x12, 0x34,                         // packet_id
            0x00,                               // reason_code
        };
        auto [b, e] = am::make_packet_range(cbs);
        BOOST_TEST(std::equal(b, e, std::begin(expected)));

        am::buffer buf{std::begin(expected), std::end(expected)};
        am::error_code ec;
        auto p = am::v5::pubrec_packet{buf, ec};
        BOOST_TEST(!ec);
        BOOST_TEST(p.packet_id() == 0x1234);
        BOOST_TEST(p.code() == am::pubrec_reason_code::success);
        BOOST_TEST(p.props().empty());

        auto cbs2 = p.const_buffer_sequence();
        BOOST_TEST(cbs2.size() == p.num_of_const_buffer_sequence());
        auto [b2, e2] = am::make_packet_range(cbs2);
        BOOST_TEST(std::equal(b2, e2, std::begin(expected)));
    }
    BOOST_TEST(
        boost::lexical_cast<std::string>(p) ==
        "v5::pubrec{pid:4660,rc:success}"
    );
}

BOOST_AUTO_TEST_CASE(v5_pubrec_prop_len_last) {
    char expected[] {
        0x50,                               // fixed_header
        0x04,                               // remaining_length
        0x12, 0x34,                         // packet_id
        0x00,                               // reason_code
        0x00,                               // property_length
    };
    am::buffer buf{std::begin(expected), std::end(expected)};
    am::error_code ec;
    auto p = am::v5::pubrec_packet{buf, ec};
    BOOST_TEST(!ec);
    BOOST_TEST(p.packet_id() == 0x1234);
    BOOST_TEST(p.code() == am::pubrec_reason_code::success);
    BOOST_TEST(p.props().empty());

    auto cbs = p.const_buffer_sequence();
    BOOST_TEST(cbs.size() == p.num_of_const_buffer_sequence());
    auto [b, e] = am::make_packet_range(cbs);
    BOOST_TEST(std::equal(b, e, std::begin(expected)));
    BOOST_TEST(
        boost::lexical_cast<std::string>(p) ==
        "v5::pubrec{pid:4660,rc:success}"
    );
}

BOOST_AUTO_TEST_CASE(v5_pubrec_error) {
    {
        am::buffer buf; // empty
        am::error_code ec;
        am::v5::pubrec_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        //                CP
        am::buffer buf{"\x00"sv}; // invalid type
        am::error_code ec;
        am::v5::pubrec_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        //                CP
        am::buffer buf{"\x50"sv}; // short
        am::error_code ec;
        am::v5::pubrec_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        //                CP  RL
        am::buffer buf{"\x50\x01"sv}; // remaining length buf mismatch
        am::error_code ec;
        am::v5::pubrec_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        //                CP  RL
        am::buffer buf{"\x50\x03"sv}; // invalid remaining length
        am::error_code ec;
        am::v5::pubrec_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        //                CP  RL  PID
        am::buffer buf{"\x50\x02\x12\x34"sv}; // valid
        am::error_code ec;
        am::v5::pubrec_packet{buf, ec};
        BOOST_TEST(ec == am::errc::success);
    }
    {
        //                CP  RL  PID
        am::buffer buf{"\x50\x02\x12\x34\x00"sv}; // remaining length mismatch
        am::error_code ec;
        am::v5::pubrec_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        //                CP  RL  PID     RC
        am::buffer buf{"\x50\x03\x12\x34\x00"sv}; // valid
        am::error_code ec;
        am::v5::pubrec_packet{buf, ec};
        BOOST_TEST(ec == am::errc::success);
    }
    {
        //                CP  RL  PID     RC
        am::buffer buf{"\x50\x03\x12\x34\x84"sv}; // invalid rc
        am::error_code ec;
        am::v5::pubrec_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        //                CP  RL  PID     RC
        am::buffer buf{"\x50\x03\x12\x34\x00\x00"sv}; // remaining length mismatch
        am::error_code ec;
        am::v5::pubrec_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        //                CP  RL  PID     RC  PL
        am::buffer buf{"\x50\x04\x12\x34\x00\x00"sv}; // valid
        am::error_code ec;
        am::v5::pubrec_packet{buf, ec};
        BOOST_TEST(ec == am::errc::success);
    }
    {
        //                CP  RL  PID     RC  PL
        am::buffer buf{"\x50\x05\x12\x34\x00\x00\x00"sv}; // property length mismatch
        am::error_code ec;
        am::v5::pubrec_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        //                CP  RL  PID     RC  PL
        am::buffer buf{"\x50\x05\x12\x34\x00\x02\x00"sv}; // property length mismatch
        am::error_code ec;
        am::v5::pubrec_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        //                CP  RL  PID     RC  PL
        am::buffer buf{"\x50\x07\x12\x34\x00\xff\xff\xff\x80"sv}; // over property length
        am::error_code ec;
        am::v5::pubrec_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
}

BOOST_AUTO_TEST_SUITE_END()
