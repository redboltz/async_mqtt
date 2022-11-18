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

BOOST_AUTO_TEST_SUITE(ut_ep_size_max)

namespace am = async_mqtt;
namespace as = boost::asio;

BOOST_AUTO_TEST_CASE(client_send) {
    auto version = am::protocol_version::v5;
    as::io_context ioc;
    auto guard = as::make_work_guard(ioc.get_executor());
    std::thread th {
        [&] {
            ioc.run();
        }
    };

    am::endpoint<async_mqtt::role::client, async_mqtt::stub_socket> ep{
        version,
        // for stub_socket args
        version,
        ioc
    };

    auto connect = am::v5::connect_packet{
        true,   // clean_start
        0x1234, // keep_alive
        am::allocate_buffer("cid1"),
        am::nullopt, // will
        am::allocate_buffer("user1"),
        am::allocate_buffer("pass1"),
        am::properties{
            am::property::session_expiry_interval{am::session_never_expire}
        }
    };

    auto connack = am::v5::connack_packet{
        true,   // session_present
        am::connect_reason_code::success,
        am::properties{
            am::property::maximum_packet_size{21}
        }
    };

    // size: 21bytes
    auto pid_opt1 = ep.acquire_unique_packet_id(as::use_future).get();
    BOOST_TEST(pid_opt1.has_value());
    auto publish_1_q1 = am::v5::publish_packet(
        *pid_opt1,
        am::allocate_buffer("topic1"),
        am::allocate_buffer("payload1"),
        am::qos::at_least_once,
        am::properties{}
    );
    std::cout << am::hex_dump(publish_1_q1) << std::endl;

    // size: 22bytes
    auto pid_opt2 = ep.acquire_unique_packet_id(as::use_future).get();
    BOOST_TEST(pid_opt2.has_value());
    auto publish_2_q1 = am::v5::publish_packet(
        *pid_opt2,
        am::allocate_buffer("topic1"),
        am::allocate_buffer("payload1+"),
        am::qos::at_least_once,
        am::properties{}
    );

    auto close = am::make_error(am::errc::network_reset, "pseudo close");

    ep.stream().next_layer().set_recv_packets(
        {
            // receive packets
            connack,
            close,
            connack,
        }
    );

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

    // send publish_1
    ep.stream().next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(publish_1_q1, wp));
        }
    );
    {
        auto ec = ep.send(publish_1_q1, as::use_future).get();
        BOOST_TEST(!ec);
    }

    // send publish_2
    ep.stream().next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(publish_2_q1, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::bad_message);
    }

    guard.reset();
    th.join();
}

BOOST_AUTO_TEST_SUITE_END()
