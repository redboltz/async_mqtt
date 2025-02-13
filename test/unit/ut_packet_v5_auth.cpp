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
struct v5_auth;
struct v5_auth_no_arg;
struct v5_auth_pid_rc;
struct v5_auth_prop_len_last;
struct v5_auth_error;
BOOST_AUTO_TEST_SUITE_END()

#include <async_mqtt/protocol/packet/v5_auth.hpp>
#include <async_mqtt/protocol/packet/packet_iterator.hpp>
#include <async_mqtt/protocol/packet/packet_traits.hpp>

BOOST_AUTO_TEST_SUITE(ut_packet)

namespace am = async_mqtt;
using namespace std::literals::string_view_literals;

BOOST_AUTO_TEST_CASE(v5_auth) {
    BOOST_TEST(am::is_auth<am::v5::auth_packet>());
    BOOST_TEST(!am::is_v3_1_1<am::v5::auth_packet>());
    BOOST_TEST(am::is_v5<am::v5::auth_packet>());
    BOOST_TEST(am::is_client_sendable<am::v5::auth_packet>());
    BOOST_TEST(am::is_server_sendable<am::v5::auth_packet>());

    auto props = am::properties{
        am::property::reason_string("some reason")
    };
    auto p = am::v5::auth_packet{
        am::auth_reason_code::continue_authentication,
        props
    };
    BOOST_TEST(p.code() == am::auth_reason_code::continue_authentication);
    BOOST_TEST(p.props() == props);
    {
        auto cbs = p.const_buffer_sequence();
        BOOST_TEST(cbs.size() == p.num_of_const_buffer_sequence());
        char expected[] {
            char(0xf0),                         // fixed_header
            0x10,                               // remaining_length
            0x18,                               // reason_code
            0x0e,                               // property_length
            0x1f,                               // reason_string
            0x00, 0x0b, 0x73, 0x6f, 0x6d, 0x65, 0x20, 0x72, 0x65, 0x61, 0x73, 0x6f, 0x6e,
        };
        auto [b, e] = am::make_packet_range(cbs);
        BOOST_TEST(std::equal(b, e, std::begin(expected)));

        am::buffer buf{std::begin(expected), std::end(expected)};
        am::error_code ec;
        auto p = am::v5::auth_packet{buf, ec};
        BOOST_TEST(!ec);
        BOOST_TEST(p.code() == am::auth_reason_code::continue_authentication);
        BOOST_TEST(p.props() == props);

        auto cbs2 = p.const_buffer_sequence();
        BOOST_TEST(cbs2.size() == p.num_of_const_buffer_sequence());
        auto [b2, e2] = am::make_packet_range(cbs2);
        BOOST_TEST(std::equal(b2, e2, std::begin(expected)));
    }
    BOOST_TEST(
        boost::lexical_cast<std::string>(p) ==
        "v5::auth{rc:continue_authentication,ps:[{id:reason_string,val:some reason}]}"
    );

    auto p2 = am::v5::auth_packet{
        am::auth_reason_code::continue_authentication,
        props
    };
    auto p3 = am::v5::auth_packet{
        am::auth_reason_code::success,
        props
    };
    BOOST_CHECK(p == p2);
    BOOST_CHECK(!(p < p2));
    BOOST_CHECK(!(p2< p));
    BOOST_CHECK(p < p3 || p3 < p);

    try {
        auto p = am::v5::auth_packet{
            am::auth_reason_code::continue_authentication,
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

BOOST_AUTO_TEST_CASE(v5_auth_no_arg) {
    auto p = am::v5::auth_packet{};
    BOOST_TEST(p.code() == am::auth_reason_code::success);
    BOOST_TEST(p.props().empty());

    {
        auto cbs = p.const_buffer_sequence();
        BOOST_TEST(cbs.size() == p.num_of_const_buffer_sequence());
        char expected[] {
            char(0xf0),                         // fixed_header
            0x00,                               // remaining_length
        };
        auto [b, e] = am::make_packet_range(cbs);
        BOOST_TEST(std::equal(b, e, std::begin(expected)));

        am::buffer buf{std::begin(expected), std::end(expected)};
        am::error_code ec;
        auto p = am::v5::auth_packet{buf, ec};
        BOOST_TEST(!ec);
        BOOST_TEST(p.code() == am::auth_reason_code::success);
        BOOST_TEST(p.props().empty());

        auto cbs2 = p.const_buffer_sequence();
        BOOST_TEST(cbs2.size() == p.num_of_const_buffer_sequence());
        auto [b2, e2] = am::make_packet_range(cbs2);
        BOOST_TEST(std::equal(b2, e2, std::begin(expected)));

        BOOST_TEST(p.type() == am::control_packet_type::auth);
    }
    BOOST_TEST(
        boost::lexical_cast<std::string>(p) ==
        "v5::auth{}"
    );
}

BOOST_AUTO_TEST_CASE(v5_auth_pid_rc) {
    auto p = am::v5::auth_packet{
        am::auth_reason_code::success
    };
    BOOST_TEST(p.code() == am::auth_reason_code::success);
    BOOST_TEST(p.props().empty());

    {
        auto cbs = p.const_buffer_sequence();
        BOOST_TEST(cbs.size() == p.num_of_const_buffer_sequence());
        char expected[] {
            char(0xf0),                         // fixed_header
            0x01,                               // remaining_length
            0x00,                               // reason_code
        };
        auto [b, e] = am::make_packet_range(cbs);
        BOOST_TEST(std::equal(b, e, std::begin(expected)));

        am::buffer buf{std::begin(expected), std::end(expected)};
        am::error_code ec;
        auto p = am::v5::auth_packet{buf, ec};
        BOOST_TEST(!ec);
        BOOST_TEST(p.code() == am::auth_reason_code::success);
        BOOST_TEST(p.props().empty());

        auto cbs2 = p.const_buffer_sequence();
        BOOST_TEST(cbs2.size() == p.num_of_const_buffer_sequence());
        auto [b2, e2] = am::make_packet_range(cbs2);
        BOOST_TEST(std::equal(b2, e2, std::begin(expected)));
    }
    BOOST_TEST(
        boost::lexical_cast<std::string>(p) ==
        "v5::auth{rc:success}"
    );
}

BOOST_AUTO_TEST_CASE(v5_auth_prop_len_last) {
    char expected[] {
        char(0xf0),                         // fixed_header
        0x02,                               // remaining_length
        0x00,                               // reason_code
        0x00,                               // property_length
    };
    am::buffer buf{std::begin(expected), std::end(expected)};
    am::error_code ec;
    auto p = am::v5::auth_packet{buf, ec};
    BOOST_TEST(!ec);
    BOOST_TEST(p.code() == am::auth_reason_code::success);
    BOOST_TEST(p.props().empty());

    auto cbs = p.const_buffer_sequence();
    BOOST_TEST(cbs.size() == p.num_of_const_buffer_sequence());
    auto [b, e] = am::make_packet_range(cbs);
    BOOST_TEST(std::equal(b, e, std::begin(expected)));
    BOOST_TEST(
        boost::lexical_cast<std::string>(p) ==
        "v5::auth{rc:success}"
    );
}

BOOST_AUTO_TEST_CASE(v5_auth_error) {
    {
        am::buffer buf; // empty
        am::error_code ec;
        am::v5::auth_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        //                CP
        am::buffer buf{"\x00"sv}; // invalid type
        am::error_code ec;
        am::v5::auth_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        //                CP
        am::buffer buf{"\xf0"sv}; // short
        am::error_code ec;
        am::v5::auth_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        //                CP  RL
        am::buffer buf{"\xf0\x01"sv}; // remaining length buf mismatch
        am::error_code ec;
        am::v5::auth_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        //                CP  RL
        am::buffer buf{"\xf0\x00"sv}; // valid
        am::error_code ec;
        am::v5::auth_packet{buf, ec};
        BOOST_TEST(ec == am::error_code{});
    }
    {
        //                CP  RL
        am::buffer buf{"\xf0\x00\x00"sv}; // too long
        am::error_code ec;
        am::v5::auth_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        //                CP  RL  RC
        am::buffer buf{"\xf0\x01\x92"sv}; // invalid rc
        am::error_code ec;
        am::v5::auth_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        //                CP  RL  RC
        am::buffer buf{"\xf0\x01\x18"sv}; // valid
        am::error_code ec;
        am::v5::auth_packet{buf, ec};
        BOOST_TEST(ec == am::error_code{});
    }
    {
        //                CP  RL  RC  PL
        am::buffer buf{"\xf0\x02\x18\x00"sv}; // valid
        am::error_code ec;
        am::v5::auth_packet{buf, ec};
        BOOST_TEST(ec == am::error_code{});
    }
    {
        //                CP  RL  RC  PL
        am::buffer buf{"\xf0\x02\x18\x00\x00"sv}; // too long
        am::error_code ec;
        am::v5::auth_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        //                CP  RL  RC  PL
        am::buffer buf{"\xf0\x02\x18\x80"sv}; // invalid property length
        am::error_code ec;
        am::v5::auth_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    {
        //                CP  RL  RC  PL
        am::buffer buf{"\xf0\x02\x18\x01"sv}; // property length mismatch
        am::error_code ec;
        am::v5::auth_packet{buf, ec};
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
}

BOOST_AUTO_TEST_SUITE_END()
