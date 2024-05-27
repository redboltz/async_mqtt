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
#include <async_mqtt/util/packet_variant_operator.hpp>

BOOST_AUTO_TEST_SUITE(ut_ep_keep_alive)

namespace am = async_mqtt;
namespace as = boost::asio;

BOOST_AUTO_TEST_CASE(server_keep_alive) {
    auto version = am::protocol_version::v5;
    as::io_context ioc;
    auto guard = as::make_work_guard(ioc.get_executor());
    std::thread th {
        [&] {
            ioc.run();
        }
    };

    auto ep = am::endpoint<async_mqtt::role::client, async_mqtt::stub_socket>::create(
        version,
        // for stub_socket args
        version,
        ioc.get_executor()
    );

    auto connect = am::v5::connect_packet{
        true,   // clean_start
        0,      // keep_alive no pingreq sending
        "cid1",
        std::nullopt, // will
        "user1",
        "pass1",
        am::properties{}
    };

    auto connack = am::v5::connack_packet{
        true,   // session_present
        am::connect_reason_code::success,
        am::properties{
            am::property::server_keep_alive{1}, // override keepalive to 1
        }
    };

    auto disconnect = am::v5::disconnect_packet{};

    auto pingreq = am::v5::pingreq_packet();

    ep->next_layer().set_recv_packets(
        {
            // receive packets
            {connack},
            {am::errc::make_error_code(am::errc::connection_reset)},
        }
    );

    // send connect
    ep->next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(connect == wp);
        }
    );
    {
        auto [ec] = ep->async_send(connect, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }

    // recv connack
    {
        auto [ec, pv] = ep->async_recv(as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
        BOOST_TEST(connack == pv);
    }

    std::promise<void> pro;
    auto fut = pro.get_future();
    // send pingreq packet due to overridden keepalive
    ep->next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(pingreq == wp);
            pro.set_value();
        }
    );
    fut.get();

    // send disconnect
    ep->next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(disconnect == wp);
        }
    );
    {
        auto [ec] = ep->async_send(disconnect, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }

    // recv close
    {
        auto [ec, pv] = ep->async_recv(as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::errc::connection_reset);
        BOOST_TEST(!pv);
    }

    guard.reset();
    th.join();
}

BOOST_AUTO_TEST_SUITE_END()
