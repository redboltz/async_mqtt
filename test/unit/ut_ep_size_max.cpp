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

    ep.next_layer().set_close_checker(
        [&] { BOOST_TEST(false); }
    );

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

    ep.next_layer().set_recv_packets(
        {
            // receive packets
            connack,
        }
    );

    // send connect
    ep.next_layer().set_write_packet_checker(
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

    // send publish_1
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(publish_1_q1, wp));
        }
    );
    {
        auto ec = ep.send(publish_1_q1, as::use_future).get();
        BOOST_TEST(!ec);
    }

    // send publish_2
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(publish_2_q1, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::bad_message);
    }
    auto pid_opt3 = ep.acquire_unique_packet_id(as::use_future).get();
    BOOST_TEST(pid_opt3.has_value());
    BOOST_TEST(*pid_opt3 == 2); // 2 can be resused

    ep.next_layer().set_close_checker({});
    ep.close(as::use_future).get();
    guard.reset();
    th.join();
}

BOOST_AUTO_TEST_CASE(client_recv) {
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

    ep.next_layer().set_close_checker(
        [&] { BOOST_TEST(false); }
    );

    auto connect = am::v5::connect_packet{
        true,   // clean_start
        0x1234, // keep_alive
        am::allocate_buffer("cid1"),
        am::nullopt, // will
        am::allocate_buffer("user1"),
        am::allocate_buffer("pass1"),
        am::properties{
            am::property::session_expiry_interval{am::session_never_expire},
            am::property::maximum_packet_size{21}
        }
    };

    auto connack = am::v5::connack_packet{
        true,   // session_present
        am::connect_reason_code::success,
        am::properties{
        }
    };

    auto disconnect = am::v5::disconnect_packet{
        am::disconnect_reason_code::packet_too_large,
        am::properties{}
    };

    ep.next_layer().set_recv_packets(
        {
            // receive packets
            connack,
        }
    );

    // send connect
    ep.next_layer().set_write_packet_checker(
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

    ep.next_layer().set_recv_packets(
        {
            // receive packets
            publish_1_q1,
            publish_2_q1,
        }
    );

    // recv publish1
    {
        auto pv = ep.recv(as::use_future).get();
        BOOST_TEST(am::packet_compare(publish_1_q1, pv));
    }

    // recv publish2
    bool close_called = false;
    ep.next_layer().set_close_checker(
        [&] { close_called = true; }
    );
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(!close_called);
            BOOST_TEST(am::packet_compare(disconnect, wp));
        }
    );
    {
        auto pv = ep.recv(as::use_future).get();
        BOOST_TEST(pv.get_if<am::system_error>() != nullptr);
        BOOST_TEST(pv.get_if<am::system_error>()->code() == am::errc::bad_message);
    }
    BOOST_TEST(close_called);

    guard.reset();
    th.join();
}

BOOST_AUTO_TEST_CASE(server_recv) {
    auto version = am::protocol_version::v5;
    as::io_context ioc;
    auto guard = as::make_work_guard(ioc.get_executor());
    std::thread th {
        [&] {
            ioc.run();
        }
    };

    am::endpoint<async_mqtt::role::server, async_mqtt::stub_socket> ep{
        version,
        // for stub_socket args
        version,
        ioc
    };

    ep.next_layer().set_close_checker(
        [&] { BOOST_TEST(false); }
    );

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

    auto disconnect = am::v5::disconnect_packet{
        am::disconnect_reason_code::packet_too_large,
        am::properties{}
    };

    ep.next_layer().set_recv_packets(
        {
            // receive packets
            connect,
        }
    );

    // recv connect
    {
        auto pv = ep.recv(as::use_future).get();
        BOOST_TEST(am::packet_compare(connect, pv));
    }

    // send connack
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(connack, wp));
        }
    );
    {
        auto ec = ep.send(connack, as::use_future).get();
        BOOST_TEST(!ec);
    }

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

    ep.next_layer().set_recv_packets(
        {
            // receive packets
            publish_1_q1,
            publish_2_q1,
        }
    );

    // recv publish1
    {
        auto pv = ep.recv(as::use_future).get();
        BOOST_TEST(am::packet_compare(publish_1_q1, pv));
    }

    // recv publish2
    bool close_called = false;
    ep.next_layer().set_close_checker(
        [&] { close_called = true; }
    );
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(!close_called);
            BOOST_TEST(am::packet_compare(disconnect, wp));
        }
    );
    {
        auto pv = ep.recv(as::use_future).get();
        BOOST_TEST(pv.get_if<am::system_error>() != nullptr);
        BOOST_TEST(pv.get_if<am::system_error>()->code() == am::errc::bad_message);
    }
    BOOST_TEST(close_called);

    guard.reset();
    th.join();
}

BOOST_AUTO_TEST_SUITE_END()
