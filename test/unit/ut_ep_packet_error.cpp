// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "../common/test_main.hpp"
#include "../common/global_fixture.hpp"

#include <thread>

#include <boost/asio.hpp>

#include <async_mqtt/endpoint.hpp>

#include "stub_socket.hpp"

BOOST_AUTO_TEST_SUITE(ut_ep_packet_error)

namespace am = async_mqtt;
namespace as = boost::asio;

using namespace std::literals::string_view_literals;

BOOST_AUTO_TEST_CASE(v311_before_connected) {
    auto version = am::protocol_version::v3_1_1;
    as::io_context ioc;
    auto guard = as::make_work_guard(ioc.get_executor());
    std::thread th {
        [&] {
            ioc.run();
        }
    };

    auto ep = am::endpoint<async_mqtt::role::client, async_mqtt::stub_socket>{
        version,
        // for stub_socket args
        version,
        ioc.get_executor()
    };

    auto connect = am::v3_1_1::connect_packet{
        true,   // clean_session
        0x1234, // keep_alive
        "cid1",
        std::nullopt, // will
        "user1",
        "pass1"
    };

    ep.next_layer().set_recv_packets(
        {
            // receive packets
            {"\x20\x02\x02\x00"sv}, // invalid reserved flag
            {am::errc::make_error_code(am::errc::connection_reset)},
        }
    );

    // underlying handshake
    {
        auto [ec] = ep.async_underlying_handshake(as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }

    // send connect
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(connect == wp);
        }
    );
    {
        auto [ec] = ep.async_send(connect, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }

    // recv connack (malformed)
    {
        auto [ec, pv] = ep.async_recv(as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    // recv close
    {
        auto [ec, pv] = ep.async_recv(as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::errc::connection_reset);
        BOOST_TEST(!pv);
    }

    guard.reset();
    th.join();
}

BOOST_AUTO_TEST_CASE(v5_before_connected) {
    auto version = am::protocol_version::v5;
    as::io_context ioc;
    auto guard = as::make_work_guard(ioc.get_executor());
    std::thread th {
        [&] {
            ioc.run();
        }
    };

    auto ep = am::endpoint<async_mqtt::role::client, async_mqtt::stub_socket>{
        version,
        // for stub_socket args
        version,
        ioc.get_executor()
    };

    auto connect = am::v5::connect_packet{
        true,   // clean_start
        0x1234, // keep_alive
        "cid1",
        std::nullopt, // will
        "user1",
        "pass1",
        am::properties{}
    };

    ep.next_layer().set_recv_packets(
        {
            // receive packets
            {"\x20\x02\x02\x00"sv}, // invalid reserved flag
            {am::errc::make_error_code(am::errc::connection_reset)},
        }
    );

    // underlying handshake
    {
        auto [ec] = ep.async_underlying_handshake(as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }

    // send connect
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(connect == wp);
        }
    );
    {
        auto [ec] = ep.async_send(connect, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }

    // recv connack (malformed)
    {
        auto [ec, pv] = ep.async_recv(as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }
    // recv close
    {
        auto [ec, pv] = ep.async_recv(as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::errc::connection_reset);
        BOOST_TEST(!pv);
    }

    guard.reset();
    th.join();
}

BOOST_AUTO_TEST_CASE(v5_after_connected) {
    auto version = am::protocol_version::v5;
    as::io_context ioc;
    auto guard = as::make_work_guard(ioc.get_executor());
    std::thread th {
        [&] {
            ioc.run();
        }
    };

    auto ep = am::endpoint<async_mqtt::role::client, async_mqtt::stub_socket>{
        version,
        // for stub_socket args
        version,
        ioc.get_executor()
    };

    auto connect = am::v5::connect_packet{
        true,   // clean_start
        0x1234, // keep_alive
        "cid1",
        std::nullopt, // will
        "user1",
        "pass1",
        am::properties{}
    };

    auto connack = am::v5::connack_packet{
        true,   // session_present
        am::connect_reason_code::success,
        am::properties{}
    };

    ep.next_layer().set_recv_packets(
        {
            // receive packets
            {connack},
            {"\x20\x02\x02\x00"sv}, // invalid reserved flag
            {am::errc::make_error_code(am::errc::connection_reset)},
        }
    );

    // underlying handshake
    {
        auto [ec] = ep.async_underlying_handshake(as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }

    // send connect
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(connect == wp);
        }
    );
    {
        auto [ec] = ep.async_send(connect, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }

    // recv connack
    {
        auto [ec, pv] = ep.async_recv(as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
        BOOST_TEST(connack == *pv);
    }

    // set checker before malformed packet recv
    auto disconnect = am::v5::disconnect_packet{
        am::disconnect_reason_code::malformed_packet
    };

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(disconnect == wp);
        }
    );

    // recv connack (malformed)
    {
        auto [ec, pv] = ep.async_recv(as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::disconnect_reason_code::malformed_packet);
    }


    // recv close
    {
        auto [ec, pv] = ep.async_recv(as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::errc::connection_reset);
        BOOST_TEST(!pv);
    }

    guard.reset();
    th.join();
}

BOOST_AUTO_TEST_CASE(v5_server_connect) {
    auto version = am::protocol_version::v5;
    as::io_context ioc;
    auto guard = as::make_work_guard(ioc.get_executor());
    std::thread th {
        [&] {
            ioc.run();
        }
    };

    auto ep = am::endpoint<async_mqtt::role::server, async_mqtt::stub_socket>{
        version,
        // for stub_socket args
        version,
        ioc.get_executor()
    };
    // connection established as server
    ep.underlying_accepted();
    ep.next_layer().set_recv_packets(
        {
            // receive packets
            {"\x10\x08\x00\x04MQTT\x05\x01"sv}, // invalid reserved flags
        }
    );

    // set checker before malformed packet recv
    auto connack = am::v5::connack_packet{
        false,   // session_present
        am::connect_reason_code::malformed_packet
    };

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(connack == wp);
        }
    );

    // recv connect (malformed)
    {
        auto [ec, pv] = ep.async_recv(as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::connect_reason_code::malformed_packet);
    }

    guard.reset();
    th.join();
}


BOOST_AUTO_TEST_SUITE_END()
