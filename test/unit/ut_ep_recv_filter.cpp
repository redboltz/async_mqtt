// Copyright Takatoshi Kondo 2023
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

BOOST_AUTO_TEST_SUITE(ut_ep_recv_filter)

namespace am = async_mqtt;
using namespace am::literals;
namespace as = boost::asio;

// To test the bug fix for https://github.com/redboltz/async_mqtt/issues/42
BOOST_AUTO_TEST_CASE(recv_filter) {
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
        0x1234, // keep_alive
        "cid1",
        std::nullopt, // will
        "user1",
        "pass1",
        am::properties{}
    };

    auto connack = am::v5::connack_packet{
        false,   // session_present
        am::connect_reason_code::success,
        am::properties{
            am::property::receive_maximum{2}
        }
    };

    ep->next_layer().set_recv_packets(
        {
            // receive packets
            connack,
        }
    );

    // send connect
    ep->next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(connect == wp);
        }
    );
    {
        auto ec = ep->send(connect, as::use_future).get();
        BOOST_TEST(!ec);
    }

    // recv connack
    {
        auto pv = ep->recv(am::filter::match, {am::control_packet_type::connack}, as::use_future).get();
        BOOST_TEST(connack == pv);
    }

    ep->close(as::use_future).get();
    guard.reset();
    th.join();
}

BOOST_AUTO_TEST_SUITE_END()
