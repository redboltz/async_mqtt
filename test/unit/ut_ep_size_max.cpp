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

    auto ep = am::endpoint<async_mqtt::role::client, async_mqtt::stub_socket>::create(
        version,
        // for stub_socket args
        version,
        ioc.get_executor()
    );

    ep->next_layer().set_close_checker(
        [&] { BOOST_TEST(false); }
    );

    auto connect = am::v5::connect_packet{
        true,   // clean_start
        0x1234, // keep_alive
        "cid1",
        std::nullopt, // will
        "user1",
        "pass1",
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

    ep->next_layer().set_recv_packets(
        {
            // receive packets
            {connack},
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

    // size: 21bytes
    auto [ec1, pid1] = ep->async_acquire_unique_packet_id(as::as_tuple(as::use_future)).get();
    BOOST_TEST(!ec1);
    auto publish_1_q1 = am::v5::publish_packet(
        pid1,
        "topic1",
        "payload1",
        am::qos::at_least_once,
        am::properties{}
    );

    // size: 22bytes
    auto [ec2, pid2] = ep->async_acquire_unique_packet_id(as::as_tuple(as::use_future)).get();
    BOOST_TEST(!ec2);
    auto publish_2_q1 = am::v5::publish_packet(
        pid2,
        "topic1",
        "payload1+",
        am::qos::at_least_once,
        am::properties{}
    );

    // send publish_1
    ep->next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(publish_1_q1 == wp);
        }
    );
    {
        auto [ec] = ep->async_send(publish_1_q1, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }

    // send publish_2
    ep->next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep->async_send(publish_2_q1, as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }
    auto [ec3, pid3] = ep->async_acquire_unique_packet_id(as::as_tuple(as::use_future)).get();
    BOOST_TEST(!ec3);
    BOOST_TEST(pid3 == 2); // 2 can be resused

    ep->next_layer().set_close_checker({});
    ep->async_close(as::as_tuple(as::use_future)).get();
    guard.reset();
    th.join();
}

BOOST_AUTO_TEST_CASE(client_send_no_store) {
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

    ep->next_layer().set_close_checker(
        [&] { BOOST_TEST(false); }
    );

    auto connect = am::v5::connect_packet{
        true,   // clean_start
        0x1234, // keep_alive
        "cid1",
        std::nullopt, // will
        "user1",
        "pass1"
    };

    auto connack = am::v5::connack_packet{
        false,   // session_present
        am::connect_reason_code::success,
        am::properties{
            am::property::maximum_packet_size{21}
        }
    };

    ep->next_layer().set_recv_packets(
        {
            // receive packets
            {connack},
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

    // size: 21bytes
    auto [ec1, pid1] = ep->async_acquire_unique_packet_id(as::as_tuple(as::use_future)).get();
    BOOST_TEST(!ec1);
    auto publish_1_q1 = am::v5::publish_packet(
        pid1,
        "topic1",
        "payload1",
        am::qos::at_least_once,
        am::properties{}
    );

    // size: 22bytes
    auto [ec2, pid2] = ep->async_acquire_unique_packet_id(as::as_tuple(as::use_future)).get();
    BOOST_TEST(!ec2);
    auto publish_2_q1 = am::v5::publish_packet(
        pid2,
        "topic1",
        "payload1+",
        am::qos::at_least_once,
        am::properties{}
    );

    // send publish_1
    ep->next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(publish_1_q1 == wp);
        }
    );
    {
        auto [ec] = ep->async_send(publish_1_q1, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }

    // send publish_2
    ep->next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep->async_send(publish_2_q1, as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }
    auto [ec3, pid3] = ep->async_acquire_unique_packet_id(as::as_tuple(as::use_future)).get();
    BOOST_TEST(!ec3);
    BOOST_TEST(pid3 == 2); // 2 can be resused

    ep->next_layer().set_close_checker({});
    ep->async_close(as::as_tuple(as::use_future)).get();
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

    auto ep = am::endpoint<async_mqtt::role::client, async_mqtt::stub_socket>::create(
        version,
        // for stub_socket args
        version,
        ioc.get_executor()
    );

    ep->next_layer().set_close_checker(
        [&] { BOOST_TEST(false); }
    );

    auto connect = am::v5::connect_packet{
        true,   // clean_start
        0x1234, // keep_alive
        "cid1",
        std::nullopt, // will
        "user1",
        "pass1",
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

    ep->next_layer().set_recv_packets(
        {
            // receive packets
            {connack},
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

    // size: 21bytes
    auto [ec1, pid1] = ep->async_acquire_unique_packet_id(as::as_tuple(as::use_future)).get();
    BOOST_TEST(!ec1);
    auto publish_1_q1 = am::v5::publish_packet(
        pid1,
        "topic1",
        "payload1",
        am::qos::at_least_once,
        am::properties{}
    );

    // size: 22bytes
    auto [ec2, pid2] = ep->async_acquire_unique_packet_id(as::as_tuple(as::use_future)).get();
    BOOST_TEST(!ec2);
    auto publish_2_q1 = am::v5::publish_packet(
        pid2,
        "topic1",
        "payload1+",
        am::qos::at_least_once,
        am::properties{}
    );

    ep->next_layer().set_recv_packets(
        {
            // receive packets
            {publish_1_q1},
            {publish_2_q1},
        }
    );

    // recv publish1
    {
        auto [ec, pv] = ep->async_recv(as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
        BOOST_TEST(publish_1_q1 == pv);
    }

    // recv publish2
    bool close_called = false;
    ep->next_layer().set_close_checker(
        [&] { close_called = true; }
    );
    ep->next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(!close_called);
            BOOST_TEST(disconnect == wp);
        }
    );
    {
        auto [ec, pv] = ep->async_recv(as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::disconnect_reason_code::packet_too_large);
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

    auto ep = am::endpoint<async_mqtt::role::server, async_mqtt::stub_socket>::create(
        version,
        // for stub_socket args
        version,
        ioc.get_executor()
    );

    ep->next_layer().set_close_checker(
        [&] { BOOST_TEST(false); }
    );

    auto connect = am::v5::connect_packet{
        true,   // clean_start
        0x1234, // keep_alive
        "cid1",
        std::nullopt, // will
        "user1",
        "pass1",
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

    ep->next_layer().set_recv_packets(
        {
            // receive packets
            {connect},
        }
    );

    // recv connect
    {
        auto [ec, pv] = ep->async_recv(as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
        BOOST_TEST(connect == pv);
    }

    // send connack
    ep->next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(connack == wp);
        }
    );
    {
        auto [ec] = ep->async_send(connack, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }

    // size: 21bytes
    auto [ec1, pid1] = ep->async_acquire_unique_packet_id(as::as_tuple(as::use_future)).get();
    BOOST_TEST(!ec1);
    auto publish_1_q1 = am::v5::publish_packet(
        pid1,
        "topic1",
        "payload1",
        am::qos::at_least_once,
        am::properties{}
    );

    // size: 22bytes
    auto [ec2, pid2] = ep->async_acquire_unique_packet_id(as::as_tuple(as::use_future)).get();
    BOOST_TEST(!ec2);
    auto publish_2_q1 = am::v5::publish_packet(
        pid2,
        "topic1",
        "payload1+",
        am::qos::at_least_once,
        am::properties{}
    );

    ep->next_layer().set_recv_packets(
        {
            // receive packets
            {publish_1_q1},
            {publish_2_q1},
        }
    );

    // recv publish1
    {
        auto [ec, pv] = ep->async_recv(as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
        BOOST_TEST(publish_1_q1 == pv);
    }

    // recv publish2
    bool close_called = false;
    ep->next_layer().set_close_checker(
        [&] { close_called = true; }
    );
    ep->next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(!close_called);
            BOOST_TEST(disconnect == wp);
        }
    );
    {
        auto [ec, pv] = ep->async_recv(as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::disconnect_reason_code::packet_too_large);
    }
    BOOST_TEST(close_called);

    guard.reset();
    th.join();
}

BOOST_AUTO_TEST_SUITE_END()
