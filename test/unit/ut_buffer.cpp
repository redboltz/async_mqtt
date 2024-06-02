// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "../common/test_main.hpp"
#include "../common/global_fixture.hpp"

#include <vector>
#include <async_mqtt/util/buffer.hpp>
#include <async_mqtt/packet/packet_helper.hpp>

#include <async_mqtt/impl/buffer_to_packet_variant.ipp>

BOOST_AUTO_TEST_SUITE(ut_buffer)

namespace am = async_mqtt;
using namespace std::literals::string_view_literals;

BOOST_AUTO_TEST_CASE( range ) {
    std::string s{"01234"};
    am::buffer buf{s.begin(), s.end()};
    BOOST_TEST(buf == "01234");
    BOOST_TEST(!buf.has_life());
    auto ss1 = buf.substr(2, 3);
    BOOST_TEST(ss1 == "234");
    BOOST_TEST(!ss1.has_life());
    auto ss2 = ss1.substr(1);
    BOOST_TEST(ss2 == "34");
    BOOST_TEST(!ss2.has_life());
}

BOOST_AUTO_TEST_CASE( view ) {
    std::string s{"01234"};
    am::buffer buf{ std::string_view{s} };
    BOOST_TEST(buf == "01234");
    BOOST_TEST(!buf.has_life());
    auto ss1 = buf.substr(2, 3);
    BOOST_TEST(ss1 == "234");
    BOOST_TEST(!ss1.has_life());
    auto ss2 = ss1.substr(1);
    BOOST_TEST(ss2 == "34");
    BOOST_TEST(!ss2.has_life());
}

BOOST_AUTO_TEST_CASE( string ) {
    {
        am::buffer moved;
        {
            am::buffer buf{ std::string{"01234"} };
            BOOST_TEST(buf == "01234");
            BOOST_TEST(buf.has_life());
            auto ss1 = buf.substr(2, 3);
            BOOST_TEST(ss1 == "234");
            BOOST_TEST(ss1.has_life());
            auto ss2 = ss1.substr(1);
            BOOST_TEST(ss2 == "34");
            BOOST_TEST(ss2.has_life());
            moved = am::force_move(buf);
        }
        BOOST_TEST(moved == "01234");
    }
    {
        am::buffer moved;
        {
            am::buffer buf{ std::string{"0123456789abcdefghijklmnopqrstuvwxyz"} };
            BOOST_TEST(buf == "0123456789abcdefghijklmnopqrstuvwxyz");
            BOOST_TEST(buf.has_life());
            auto ss1 = buf.substr(2, 3);
            BOOST_TEST(ss1 == "234");
            BOOST_TEST(ss1.has_life());
            auto ss2 = ss1.substr(1);
            BOOST_TEST(ss2 == "34");
            BOOST_TEST(ss2.has_life());
            moved = am::force_move(buf);
        }
        BOOST_TEST(moved == "0123456789abcdefghijklmnopqrstuvwxyz");
    }
}

BOOST_AUTO_TEST_CASE( buffers ) {
    std::vector<am::buffer> bufs;
    BOOST_TEST(am::to_string(bufs).empty());

    bufs.emplace_back("01234");
    BOOST_TEST(am::to_string(bufs) == "01234");

    bufs.emplace_back("5678");
    BOOST_TEST(am::to_string(bufs) == "012345678");
}

BOOST_AUTO_TEST_CASE( buf_to_pv_size_error ) {
    am::error_code ec;
    auto pv = am::buffer_to_packet_variant(am::buffer{}, am::protocol_version::v3_1_1, ec);
    BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
}

BOOST_AUTO_TEST_CASE( buf_to_pv_reserved_error ) {
    am::error_code ec;
    auto pv = am::buffer_to_packet_variant(am::buffer{"\x00\x00"sv}, am::protocol_version::undetermined, ec);
    BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
}

BOOST_AUTO_TEST_CASE( buf_to_pv_connect_version_error ) {
    am::error_code ec;
    auto pv = am::buffer_to_packet_variant(am::buffer{"\x10\x07\x00\x04MQTT\x06"sv}, am::protocol_version::undetermined, ec);
    BOOST_TEST(ec == am::connect_reason_code::unsupported_protocol_version);
}

BOOST_AUTO_TEST_CASE( buf_to_pv_packet_throw_error ) {
    am::error_code ec;
    auto pv = am::buffer_to_packet_variant(am::buffer{"\x10\x07\x00\x04MQTT\x05"sv}, am::protocol_version::undetermined, ec);
    BOOST_TEST(ec == am::connect_reason_code::malformed_packet);
}

BOOST_AUTO_TEST_CASE( buf_to_pv_connack_undetermined_error ) {
    am::error_code ec;
    auto pv = am::buffer_to_packet_variant(am::buffer{"\x20\x00"sv}, am::protocol_version::undetermined, ec);
    BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
}

BOOST_AUTO_TEST_CASE( buf_to_pv_publish_undetermined_error ) {
    am::error_code ec;
    auto pv = am::buffer_to_packet_variant(am::buffer{"\x30\x00"sv}, am::protocol_version::undetermined, ec);
    BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
}

BOOST_AUTO_TEST_CASE( buf_to_pv_puback_undetermined_error ) {
    am::error_code ec;
    auto pv = am::buffer_to_packet_variant(am::buffer{"\x40\x00"sv}, am::protocol_version::undetermined, ec);
    BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
}

BOOST_AUTO_TEST_CASE( buf_to_pv_pubrec_undetermined_error ) {
    am::error_code ec;
    auto pv = am::buffer_to_packet_variant(am::buffer{"\x50\x00"sv}, am::protocol_version::undetermined, ec);
    BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
}

BOOST_AUTO_TEST_CASE( buf_to_pv_pubrel_undetermined_error ) {
    am::error_code ec;
    auto pv = am::buffer_to_packet_variant(am::buffer{"\x60\x00"sv}, am::protocol_version::undetermined, ec);
    BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
}

BOOST_AUTO_TEST_CASE( buf_to_pv_pubcomp_undetermined_error ) {
    am::error_code ec;
    auto pv = am::buffer_to_packet_variant(am::buffer{"\x70\x00"sv}, am::protocol_version::undetermined, ec);
    BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
}

BOOST_AUTO_TEST_CASE( buf_to_pv_subscribe_undetermined_error ) {
    am::error_code ec;
    auto pv = am::buffer_to_packet_variant(am::buffer{"\x80\x00"sv}, am::protocol_version::undetermined, ec);
    BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
}

BOOST_AUTO_TEST_CASE( buf_to_pv_suback_undetermined_error ) {
    am::error_code ec;
    auto pv = am::buffer_to_packet_variant(am::buffer{"\x90\x00"sv}, am::protocol_version::undetermined, ec);
    BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
}

BOOST_AUTO_TEST_CASE( buf_to_pv_unsubscribe_undetermined_error ) {
    am::error_code ec;
    auto pv = am::buffer_to_packet_variant(am::buffer{"\xa0\x00"sv}, am::protocol_version::undetermined, ec);
    BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
}

BOOST_AUTO_TEST_CASE( buf_to_pv_unsuback_undetermined_error ) {
    am::error_code ec;
    auto pv = am::buffer_to_packet_variant(am::buffer{"\xb0\x00"sv}, am::protocol_version::undetermined, ec);
    BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
}

BOOST_AUTO_TEST_CASE( buf_to_pv_pingreq_undetermined_error ) {
    am::error_code ec;
    auto pv = am::buffer_to_packet_variant(am::buffer{"\xc0\x00"sv}, am::protocol_version::undetermined, ec);
    BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
}

BOOST_AUTO_TEST_CASE( buf_to_pv_pingresp_undetermined_error ) {
    am::error_code ec;
    auto pv = am::buffer_to_packet_variant(am::buffer{"\xd0\x00"sv}, am::protocol_version::undetermined, ec);
    BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
}

BOOST_AUTO_TEST_CASE( buf_to_pv_disconnect_undetermined_error ) {
    am::error_code ec;
    auto pv = am::buffer_to_packet_variant(am::buffer{"\xe0\x00"sv}, am::protocol_version::undetermined, ec);
    BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
}

BOOST_AUTO_TEST_CASE( buf_to_pv_auth_undetermined_error ) {
    am::error_code ec;
    auto pv = am::buffer_to_packet_variant(am::buffer{"\xf0\x00"sv}, am::protocol_version::undetermined, ec);
    BOOST_TEST(ec == am::disconnect_reason_code::protocol_error);
}

BOOST_AUTO_TEST_SUITE_END()
