// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "../common/test_main.hpp"
#include "../common/global_fixture.hpp"

#include <vector>
#include <async_mqtt/buffer.hpp>
#include <async_mqtt/buffer_to_packet_variant.hpp>
#include <async_mqtt/packet/packet_iterator.hpp>

BOOST_AUTO_TEST_SUITE(ut_buffer)

namespace am = async_mqtt;

BOOST_AUTO_TEST_CASE(seq) {
    BOOST_TEST(am::is_buffer_sequence<am::buffer>::value);
    BOOST_TEST(am::is_buffer_sequence<std::vector<am::buffer>>::value);
    BOOST_TEST(am::is_buffer_sequence<std::decay_t<std::vector<am::buffer> const&>>::value);
}

BOOST_AUTO_TEST_CASE( allocate ) {
    std::string s{"01234"};
    auto buf = am::allocate_buffer(s.begin(), s.end());
    BOOST_TEST(buf == "01234");
    BOOST_TEST(buf.has_life());
    auto ss1 = buf.substr(2, 3);
    BOOST_TEST(ss1 == "234");
    BOOST_TEST(ss1.has_life());
    auto ss2 = ss1.substr(1);
    BOOST_TEST(ss2 == "34");
    BOOST_TEST(ss2.has_life());
}

BOOST_AUTO_TEST_CASE( literals1 ) {
    auto buf = "01234";
    BOOST_TEST(buf == "01234");
    BOOST_TEST(!buf.has_life());
    auto ss1 = buf.substr(2, 3);
    BOOST_TEST(ss1 == "234");
    BOOST_TEST(!ss1.has_life());
    auto ss2 = ss1.substr(1);
    BOOST_TEST(ss2 == "34");
    BOOST_TEST(!ss2.has_life());
}

BOOST_AUTO_TEST_CASE( literals2 ) {
    using namespace std::literals::string_view_literals;
    auto buf1 = "abcde";
    BOOST_TEST(buf1.size() == 5);
    BOOST_TEST(buf1 == "abcde"sv);

    auto buf2 = "ab\0cde";
    BOOST_TEST(buf2.size() == 6);
    BOOST_TEST(buf2 == "ab\0cde"sv);
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
    BOOST_TEST(am::to_buffer(bufs).empty());

    bufs.emplace_back("01234");
    BOOST_TEST(am::to_string(bufs) == "01234");
    BOOST_TEST(am::to_buffer(bufs) == "01234");

    bufs.emplace_back("5678");
    BOOST_TEST(am::to_string(bufs) == "012345678");
    BOOST_TEST(am::to_buffer(bufs) == "012345678");
}

BOOST_AUTO_TEST_CASE( buf_to_pv_size_error ) {
    auto pv = am::buffer_to_packet_variant(am::buffer{}, am::protocol_version::v3_1_1);
    if (auto const* p = pv.get_if<am::system_error>()) {
        BOOST_TEST(p->code() == am::errc::bad_message);
    }
    else {
        BOOST_TEST(false);
    }
}

BOOST_AUTO_TEST_CASE( buf_to_pv_reserved_error ) {
    auto pv = am::buffer_to_packet_variant("\x00\x00", am::protocol_version::undetermined);
    if (auto const* p = pv.get_if<am::system_error>()) {
        BOOST_TEST(p->code() == am::errc::bad_message);
    }
    else {
        BOOST_TEST(false);
    }
}

BOOST_AUTO_TEST_CASE( buf_to_pv_connect_version_error ) {
    auto pv = am::buffer_to_packet_variant("\x10\x07\x00\x04MQTT\x06", am::protocol_version::undetermined);
    if (auto const* p = pv.get_if<am::system_error>()) {
        BOOST_TEST(p->code() == am::errc::bad_message);
    }
    else {
        BOOST_TEST(false);
    }
}

BOOST_AUTO_TEST_CASE( buf_to_pv_packet_throw_error ) {
    auto pv = am::buffer_to_packet_variant("\x10\x07\x00\x04MQTT\x05", am::protocol_version::undetermined);
    if (auto const* p = pv.get_if<am::system_error>()) {
        BOOST_TEST(p->code() == am::errc::bad_message);
    }
    else {
        BOOST_TEST(false);
    }
}

BOOST_AUTO_TEST_CASE( buf_to_pv_connack_undetermined_error ) {
    auto pv = am::buffer_to_packet_variant("\x20\x00", am::protocol_version::undetermined);
    if (auto const* p = pv.get_if<am::system_error>()) {
        BOOST_TEST(p->code() == am::errc::bad_message);
    }
    else {
        BOOST_TEST(false);
    }
}

BOOST_AUTO_TEST_CASE( buf_to_pv_publish_undetermined_error ) {
    auto pv = am::buffer_to_packet_variant("\x30\x00", am::protocol_version::undetermined);
    if (auto const* p = pv.get_if<am::system_error>()) {
        BOOST_TEST(p->code() == am::errc::bad_message);
    }
    else {
        BOOST_TEST(false);
    }
}

BOOST_AUTO_TEST_CASE( buf_to_pv_puback_undetermined_error ) {
    auto pv = am::buffer_to_packet_variant("\x40\x00", am::protocol_version::undetermined);
    if (auto const* p = pv.get_if<am::system_error>()) {
        BOOST_TEST(p->code() == am::errc::bad_message);
    }
    else {
        BOOST_TEST(false);
    }
}

BOOST_AUTO_TEST_CASE( buf_to_pv_pubrec_undetermined_error ) {
    auto pv = am::buffer_to_packet_variant("\x50\x00", am::protocol_version::undetermined);
    if (auto const* p = pv.get_if<am::system_error>()) {
        BOOST_TEST(p->code() == am::errc::bad_message);
    }
    else {
        BOOST_TEST(false);
    }
}

BOOST_AUTO_TEST_CASE( buf_to_pv_pubrel_undetermined_error ) {
    auto pv = am::buffer_to_packet_variant("\x60\x00", am::protocol_version::undetermined);
    if (auto const* p = pv.get_if<am::system_error>()) {
        BOOST_TEST(p->code() == am::errc::bad_message);
    }
    else {
        BOOST_TEST(false);
    }
}

BOOST_AUTO_TEST_CASE( buf_to_pv_pubcomp_undetermined_error ) {
    auto pv = am::buffer_to_packet_variant("\x70\x00", am::protocol_version::undetermined);
    if (auto const* p = pv.get_if<am::system_error>()) {
        BOOST_TEST(p->code() == am::errc::bad_message);
    }
    else {
        BOOST_TEST(false);
    }
}

BOOST_AUTO_TEST_CASE( buf_to_pv_subscribe_undetermined_error ) {
    auto pv = am::buffer_to_packet_variant("\x80\x00", am::protocol_version::undetermined);
    if (auto const* p = pv.get_if<am::system_error>()) {
        BOOST_TEST(p->code() == am::errc::bad_message);
    }
    else {
        BOOST_TEST(false);
    }
}

BOOST_AUTO_TEST_CASE( buf_to_pv_suback_undetermined_error ) {
    auto pv = am::buffer_to_packet_variant("\x90\x00", am::protocol_version::undetermined);
    if (auto const* p = pv.get_if<am::system_error>()) {
        BOOST_TEST(p->code() == am::errc::bad_message);
    }
    else {
        BOOST_TEST(false);
    }
}

BOOST_AUTO_TEST_CASE( buf_to_pv_unsubscribe_undetermined_error ) {
    auto pv = am::buffer_to_packet_variant("\xa0\x00", am::protocol_version::undetermined);
    if (auto const* p = pv.get_if<am::system_error>()) {
        BOOST_TEST(p->code() == am::errc::bad_message);
    }
    else {
        BOOST_TEST(false);
    }
}

BOOST_AUTO_TEST_CASE( buf_to_pv_unsuback_undetermined_error ) {
    auto pv = am::buffer_to_packet_variant("\xb0\x00", am::protocol_version::undetermined);
    if (auto const* p = pv.get_if<am::system_error>()) {
        BOOST_TEST(p->code() == am::errc::bad_message);
    }
    else {
        BOOST_TEST(false);
    }
}

BOOST_AUTO_TEST_CASE( buf_to_pv_pingreq_undetermined_error ) {
    auto pv = am::buffer_to_packet_variant("\xc0\x00", am::protocol_version::undetermined);
    if (auto const* p = pv.get_if<am::system_error>()) {
        BOOST_TEST(p->code() == am::errc::bad_message);
    }
    else {
        BOOST_TEST(false);
    }
}

BOOST_AUTO_TEST_CASE( buf_to_pv_pingresp_undetermined_error ) {
    auto pv = am::buffer_to_packet_variant("\xd0\x00", am::protocol_version::undetermined);
    if (auto const* p = pv.get_if<am::system_error>()) {
        BOOST_TEST(p->code() == am::errc::bad_message);
    }
    else {
        BOOST_TEST(false);
    }
}

BOOST_AUTO_TEST_CASE( buf_to_pv_disconnect_undetermined_error ) {
    auto pv = am::buffer_to_packet_variant("\xe0\x00", am::protocol_version::undetermined);
    if (auto const* p = pv.get_if<am::system_error>()) {
        BOOST_TEST(p->code() == am::errc::bad_message);
    }
    else {
        BOOST_TEST(false);
    }
}

BOOST_AUTO_TEST_CASE( buf_to_pv_auth_undetermined_error ) {
    auto pv = am::buffer_to_packet_variant("\xf0\x00", am::protocol_version::undetermined);
    if (auto const* p = pv.get_if<am::system_error>()) {
        BOOST_TEST(p->code() == am::errc::bad_message);
    }
    else {
        BOOST_TEST(false);
    }
}

BOOST_AUTO_TEST_SUITE_END()
