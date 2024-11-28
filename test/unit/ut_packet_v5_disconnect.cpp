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
struct v5_disconnect;
struct v5_disconnect_no_arg;
struct v5_disconnect_pid_rc;
struct v5_disconnect_prop_len_last;
struct v5_disconnect_error;
BOOST_AUTO_TEST_SUITE_END()

#include <async_mqtt/protocol/packet/v5_disconnect.hpp>
#include <async_mqtt/protocol/packet/packet_iterator.hpp>
#include <async_mqtt/protocol/packet/packet_traits.hpp>

BOOST_AUTO_TEST_SUITE(ut_packet)

namespace am = async_mqtt;
using namespace std::literals::string_view_literals;

BOOST_AUTO_TEST_CASE(v5_disconnect) {
    BOOST_TEST(am::is_disconnect<am::v5::disconnect_packet>());
    BOOST_TEST(!am::is_v3_1_1<am::v5::disconnect_packet>());
    BOOST_TEST(am::is_v5<am::v5::disconnect_packet>());
    BOOST_TEST(am::is_client_sendable<am::v5::disconnect_packet>());
    BOOST_TEST(am::is_server_sendable<am::v5::disconnect_packet>());

    auto props = am::properties{
        am::property::reason_string("some reason")
    };
    auto p = am::v5::disconnect_packet{
        am::disconnect_reason_code::protocol_error,
        props
    };
    BOOST_TEST(p.code() == am::disconnect_reason_code::protocol_error);
    BOOST_TEST(p.props() == props);
    {
        auto cbs = p.const_buffer_sequence();
        BOOST_TEST(cbs.size() == p.num_of_const_buffer_sequence());
        char expected[] {
            char(0xe0),                         // fixed_header
            0x10,                               // remaining_length
            char(0x82),                         // reason_code
            0x0e,                               // property_length
            0x1f,                               // reason_string
            0x00, 0x0b, 0x73, 0x6f, 0x6d, 0x65, 0x20, 0x72, 0x65, 0x61, 0x73, 0x6f, 0x6e,
        };
        auto [b, e] = am::make_packet_range(cbs);
        BOOST_TEST(std::equal(b, e, std::begin(expected)));

        am::buffer buf{std::begin(expected), std::end(expected)};
        am::error_code ec;
        auto p = am::v5::disconnect_packet{buf, ec};
        BOOST_TEST(!ec);
        BOOST_TEST(p.code() == am::disconnect_reason_code::protocol_error);
        BOOST_TEST(p.props() == props);

        auto cbs2 = p.const_buffer_sequence();
        BOOST_TEST(cbs2.size() == p.num_of_const_buffer_sequence());
        auto [b2, e2] = am::make_packet_range(cbs2);
        BOOST_TEST(std::equal(b2, e2, std::begin(expected)));

        BOOST_TEST(p.type() == am::control_packet_type::disconnect);
    }
    BOOST_TEST(
        boost::lexical_cast<std::string>(p) ==
        "v5::disconnect{rc:protocol_error,ps:[{id:reason_string,val:some reason}]}"
    );

    auto p2 = am::v5::disconnect_packet{
        am::disconnect_reason_code::protocol_error,
        props
    };
    auto p3 = am::v5::disconnect_packet{
        am::disconnect_reason_code::normal_disconnection,
        props
    };
    BOOST_CHECK(p == p2);
    BOOST_CHECK(!(p < p2));
    BOOST_CHECK(!(p2< p));
    BOOST_CHECK(p < p3 || p3 < p);

    try {
        auto p = am::v5::disconnect_packet{
            am::disconnect_reason_code::protocol_error,
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

BOOST_AUTO_TEST_CASE(v5_disconnect_no_arg) {
    auto p = am::v5::disconnect_packet{};
    BOOST_TEST(p.code() == am::disconnect_reason_code::normal_disconnection);
    BOOST_TEST(p.props().empty());

    {
        auto cbs = p.const_buffer_sequence();
        BOOST_TEST(cbs.size() == p.num_of_const_buffer_sequence());
        char expected[] {
            char(0xe0),                         // fixed_header
            0x00,                               // remaining_length
        };
        auto [b, e] = am::make_packet_range(cbs);
        BOOST_TEST(std::equal(b, e, std::begin(expected)));

        am::buffer buf{std::begin(expected), std::end(expected)};
        am::error_code ec;
        auto p = am::v5::disconnect_packet{buf, ec};
        BOOST_TEST(!ec);
        BOOST_TEST(p.code() == am::disconnect_reason_code::normal_disconnection);
        BOOST_TEST(p.props().empty());

        auto cbs2 = p.const_buffer_sequence();
        BOOST_TEST(cbs2.size() == p.num_of_const_buffer_sequence());
        auto [b2, e2] = am::make_packet_range(cbs2);
        BOOST_TEST(std::equal(b2, e2, std::begin(expected)));
    }
    BOOST_TEST(
        boost::lexical_cast<std::string>(p) ==
        "v5::disconnect{}"
    );
}

BOOST_AUTO_TEST_CASE(v5_disconnect_pid_rc) {
    auto p = am::v5::disconnect_packet{
        am::disconnect_reason_code::normal_disconnection
    };
    BOOST_TEST(p.code() == am::disconnect_reason_code::normal_disconnection);
    BOOST_TEST(p.props().empty());

    {
        auto cbs = p.const_buffer_sequence();
        BOOST_TEST(cbs.size() == p.num_of_const_buffer_sequence());
        char expected[] {
            char(0xe0),                         // fixed_header
            0x01,                               // remaining_length
            0x00,                               // reason_code
        };
        auto [b, e] = am::make_packet_range(cbs);
        BOOST_TEST(std::equal(b, e, std::begin(expected)));

        am::buffer buf{std::begin(expected), std::end(expected)};
        am::error_code ec;
        auto p = am::v5::disconnect_packet{buf, ec};
        BOOST_TEST(!ec);
        BOOST_TEST(p.code() == am::disconnect_reason_code::normal_disconnection);
        BOOST_TEST(p.props().empty());

        auto cbs2 = p.const_buffer_sequence();
        BOOST_TEST(cbs2.size() == p.num_of_const_buffer_sequence());
        auto [b2, e2] = am::make_packet_range(cbs2);
        BOOST_TEST(std::equal(b2, e2, std::begin(expected)));
    }
    BOOST_TEST(
        boost::lexical_cast<std::string>(p) ==
        "v5::disconnect{rc:normal_disconnection}"
    );
}

BOOST_AUTO_TEST_CASE(v5_disconnect_prop_len_last) {
    char expected[] {
        char(0xe0),                         // fixed_header
        0x02,                               // remaining_length
        0x00,                               // reason_code
        0x00,                               // property_length
    };
    am::buffer buf{std::begin(expected), std::end(expected)};
    am::error_code ec;
    auto p = am::v5::disconnect_packet{buf, ec};
    BOOST_TEST(!ec);
    BOOST_TEST(p.code() == am::disconnect_reason_code::normal_disconnection);
    BOOST_TEST(p.props().empty());

    auto cbs = p.const_buffer_sequence();
    BOOST_TEST(cbs.size() == p.num_of_const_buffer_sequence());
    auto [b, e] = am::make_packet_range(cbs);
    BOOST_TEST(std::equal(b, e, std::begin(expected)));
    BOOST_TEST(
        boost::lexical_cast<std::string>(p) ==
        "v5::disconnect{rc:normal_disconnection}"
    );
}

BOOST_AUTO_TEST_CASE(v5_disconnect_error) {
    {
        am::buffer buf; // empty
        am::error_code ec;
        am::v5::disconnect_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        //                CP
        am::buffer buf{"\x00"sv}; // invalid type
        am::error_code ec;
        am::v5::disconnect_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        //                CP
        am::buffer buf{"\xe0"sv}; // short
        am::error_code ec;
        am::v5::disconnect_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        //                CP  RL
        am::buffer buf{"\xe0\x00"sv}; // valid
        am::error_code ec;
        am::v5::disconnect_packet{buf, ec};
        BOOST_TEST(ec == am::errc::success);
    }
    {
        //                CP  RL  RC
        am::buffer buf{"\xe0\x01\x00"sv}; // valid
        am::error_code ec;
        am::v5::disconnect_packet{buf, ec};
        BOOST_TEST(ec == am::errc::success);
    }
    {
        //                CP  RL
        am::buffer buf{"\xe0\x00\x00"sv}; // remaining length zero buf non zero
        am::error_code ec;
        am::v5::disconnect_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        //                CP  RL
        am::buffer buf{"\xe0\x01"sv}; // invalid remaining length
        am::error_code ec;
        am::v5::disconnect_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        //                CP  RL
        am::buffer buf{"\xe0\x80\x80\x80\x80\x00"sv}; // invalid remaining length
        am::error_code ec;
        am::v5::disconnect_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        //                CP  RL  RC
        am::buffer buf{"\xe0\x01\x00"sv}; // valid
        am::error_code ec;
        am::v5::disconnect_packet{buf, ec};
        BOOST_TEST(ec == am::errc::success);
    }
    {
        //                CP  RL  RC
        am::buffer buf{"\xe0\x01\x92"sv}; // invalid rc
        am::error_code ec;
        am::v5::disconnect_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        //                CP  RL  RC  PL
        am::buffer buf{"\xe0\x02\x00\x01"sv}; // invalid property length
        am::error_code ec;
        am::v5::disconnect_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        //                CP  RL  RC  PL
        am::buffer buf{"\xe0\x02\x00\x00"sv}; // zero property length (valid)
        am::error_code ec;
        am::v5::disconnect_packet{buf, ec};
        BOOST_TEST(ec == am::errc::success);
    }
    {
        //                CP  RL  RC  PL
        am::buffer buf{"\xe0\x03\x00\x00\x00"sv}; // property length mismatch
        am::error_code ec;
        am::v5::disconnect_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        //                CP  RL  RC  PL
        am::buffer buf{"\xe0\x05\x00\xff\xff\xff\x80"sv}; // invalid property length
        am::error_code ec;
        am::v5::disconnect_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
}

BOOST_AUTO_TEST_SUITE_END()
