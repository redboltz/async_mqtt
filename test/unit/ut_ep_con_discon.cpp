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
#include <async_mqtt/util/hex_dump.hpp>

#include "stub_socket.hpp"
#include "packet_compare.hpp"

BOOST_AUTO_TEST_SUITE(ut_ep_con_discon)

namespace am = async_mqtt;
namespace as = boost::asio;

BOOST_AUTO_TEST_CASE(valid_v3_1_1) {
    auto version = am::protocol_version::v3_1_1;
    as::io_context ioc;
    auto guard = as::make_work_guard(ioc.get_executor());
    std::thread th {
        [&] {
            ioc.run();
        }
    };

    auto connect = am::v3_1_1::connect_packet{
        true,   // clean_session
        0x1234, // keep_alive
        am::allocate_buffer("cid1"),
        am::nullopt, // will
        am::allocate_buffer("user1"),
        am::allocate_buffer("pass1")
    };

    auto connack = am::v3_1_1::connack_packet{
        true,   // session_present
        am::connect_return_code::accepted
    };

    auto disconnect = am::v3_1_1::disconnect_packet{};

    auto close = am::make_error(am::errc::network_reset, "pseudo close");

    am::endpoint<async_mqtt::role::client, async_mqtt::stub_socket> ep{
        version,
        // for stub_socket args
        version,
        ioc,
        std::deque<am::packet_variant> {
            // receive packets
            connack,
            close,
            connack,
            close,
        }
    };

    // send connect
    ep.stream().next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(connect, wp));
        }
    );
    {
        auto ec = ep.send(connect, as::use_future).get();
        BOOST_TEST(!ec);
    }

    // recv connack
    {
        auto pv = ep.recv(as::use_future).get();
        BOOST_TEST(am::packet_compare(connack, pv));
    }

    // send disconnect
    ep.stream().next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(disconnect, wp));
        }
    );
    {
        auto ec = ep.send(disconnect, as::use_future).get();
        BOOST_TEST(!ec);
    }

    // recv close
    {
        auto pv = ep.recv(as::use_future).get();
        BOOST_TEST(pv.get_if<am::system_error>() != nullptr);
    }

    // send connect
    ep.stream().next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(connect, wp));
        }
    );
    {
        auto ec = ep.send(connect, as::use_future).get();
        BOOST_TEST(!ec);
    }

    // recv connack
    {
        auto pv = ep.recv(as::use_future).get();
        BOOST_TEST(am::packet_compare(connack, pv));
    }

    // send disconnect
    ep.stream().next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(disconnect, wp));
        }
    );
    {
        auto ec = ep.send(disconnect, as::use_future).get();
        BOOST_TEST(!ec);
    }

    // recv close
    {
        auto pv = ep.recv(as::use_future).get();
        BOOST_TEST(pv.get_if<am::system_error>() != nullptr);
    }

    guard.reset();
    th.join();
}

BOOST_AUTO_TEST_SUITE_END()
