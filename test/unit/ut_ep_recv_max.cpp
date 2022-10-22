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

BOOST_AUTO_TEST_SUITE(ut_ep_recv_max)

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
        am::properties{}
    };

    auto connack = am::v5::connack_packet{
        false,   // session_present
        am::connect_reason_code::success,
        am::properties{
            am::property::receive_maximum{2}
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

    auto pid_opt1 = ep.acquire_unique_packet_id(as::use_future).get();
    BOOST_TEST(pid_opt1.has_value());
    auto publish_1_q1 = am::v5::publish_packet(
        *pid_opt1,
        am::allocate_buffer("topic1"),
        am::allocate_buffer("payload1"),
        am::qos::at_least_once,
        am::properties{}
    );

    auto pid_opt2 = ep.acquire_unique_packet_id(as::use_future).get();
    BOOST_TEST(pid_opt2.has_value());
    auto publish_2_q1 = am::v5::publish_packet(
        *pid_opt2,
        am::allocate_buffer("topic1"),
        am::allocate_buffer("payload1"),
        am::qos::at_least_once,
        am::properties{}
    );

    auto pid_opt3 = ep.acquire_unique_packet_id(as::use_future).get();
    BOOST_TEST(pid_opt3.has_value());
    auto publish_3_q2 = am::v5::publish_packet(
        *pid_opt3,
        am::allocate_buffer("topic1"),
        am::allocate_buffer("payload1"),
        am::qos::exactly_once,
        am::properties{}
    );

    auto publish_4_q0 = am::v5::publish_packet(
        0x0, // packet_id
        am::allocate_buffer("topic1"),
        am::allocate_buffer("payload1"),
        am::qos::at_most_once,
        am::properties{}
    );

    auto puback2 = am::v5::puback_packet(
        *pid_opt2
    );

    auto pubrec3 = am::v5::pubrec_packet(
        *pid_opt3
    );

    auto pubrel3 = am::v5::pubrel_packet(
        *pid_opt3
    );

    auto pubcomp3 = am::v5::pubcomp_packet(
        *pid_opt3
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
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(publish_2_q1, wp));
        }
    );
    {
        auto ec = ep.send(publish_2_q1, as::use_future).get();
        BOOST_TEST(!ec);
    }

    bool pub3_send = false;
    std::promise<void> p;
    auto f = p.get_future();
    // send publish_3
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(publish_3_q2, wp));
            pub3_send = true;
            p.set_value();
        }
    );
    {
        auto ec = ep.send(publish_3_q2, as::use_future).get();
        BOOST_TEST(!ec);
        BOOST_TEST(!pub3_send);
    }

    // send publish_4
    bool pub4_send = false;
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(publish_4_q0, wp));
            pub4_send = true;
        }
    );
    {
        auto ec = ep.send(publish_4_q0, as::use_future).get();
        BOOST_TEST(!ec);
        BOOST_TEST(pub4_send);
    }

    // set send publish_3 checker again
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(publish_3_q2, wp));
            pub3_send = true;
            p.set_value();
        }
    );

    ep.next_layer().set_recv_packets(
        {
            // receive packets
            puback2,
            pubrec3,
            pubcomp3,
        }
    );

    // recv puback2
    {
        auto pv = ep.recv(as::use_future).get();
        BOOST_TEST(am::packet_compare(puback2, pv));
    }

    f.get();

    // recv pubrec3
    {
        auto pv = ep.recv(as::use_future).get();
        BOOST_TEST(am::packet_compare(pubrec3, pv));
    }

    // send pubrel3
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(pubrel3, wp));
        }
    );
    {
        auto ec = ep.send(pubrel3, as::use_future).get();
        BOOST_TEST(!ec);
    }

    // recv pubcomp3
    {
        auto pv = ep.recv(as::use_future).get();
        BOOST_TEST(am::packet_compare(pubcomp3, pv));
    }
    ep.close(as::use_future).get();
    guard.reset();
    th.join();
}

BOOST_AUTO_TEST_CASE(server_send) {
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

    auto connect = am::v5::connect_packet{
        true,   // clean_start
        0x1234, // keep_alive
        am::allocate_buffer("cid1"),
        am::nullopt, // will
        am::allocate_buffer("user1"),
        am::allocate_buffer("pass1"),
        am::properties{
            am::property::receive_maximum{2}
        }
    };

    auto connack = am::v5::connack_packet{
        false,   // session_present
        am::connect_reason_code::success,
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

    auto pid_opt1 = ep.acquire_unique_packet_id(as::use_future).get();
    BOOST_TEST(pid_opt1.has_value());
    auto publish_1_q1 = am::v5::publish_packet(
        *pid_opt1,
        am::allocate_buffer("topic1"),
        am::allocate_buffer("payload1"),
        am::qos::at_least_once,
        am::properties{}
    );

    auto pid_opt2 = ep.acquire_unique_packet_id(as::use_future).get();
    BOOST_TEST(pid_opt2.has_value());
    auto publish_2_q1 = am::v5::publish_packet(
        *pid_opt2,
        am::allocate_buffer("topic1"),
        am::allocate_buffer("payload1"),
        am::qos::at_least_once,
        am::properties{}
    );

    auto pid_opt3 = ep.acquire_unique_packet_id(as::use_future).get();
    BOOST_TEST(pid_opt3.has_value());
    auto publish_3_q2 = am::v5::publish_packet(
        *pid_opt3,
        am::allocate_buffer("topic1"),
        am::allocate_buffer("payload1"),
        am::qos::exactly_once,
        am::properties{}
    );

    auto publish_4_q0 = am::v5::publish_packet(
        0x0, // packet_id
        am::allocate_buffer("topic1"),
        am::allocate_buffer("payload1"),
        am::qos::at_most_once,
        am::properties{}
    );

    auto puback2 = am::v5::puback_packet(
        *pid_opt2
    );

    auto pubrec3 = am::v5::pubrec_packet(
        *pid_opt3
    );

    auto pubrel3 = am::v5::pubrel_packet(
        *pid_opt3
    );

    auto pubcomp3 = am::v5::pubcomp_packet(
        *pid_opt3
    );

    ep.next_layer().set_recv_packets(
        {
            // receive packets
            puback2,
            pubrec3,
            pubcomp3,
        }
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
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(publish_2_q1, wp));
        }
    );
    {
        auto ec = ep.send(publish_2_q1, as::use_future).get();
        BOOST_TEST(!ec);
    }

    bool pub3_send = false;
    std::promise<void> p;
    auto f = p.get_future();
    // send publish_3
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(publish_3_q2, wp));
            pub3_send = true;
            p.set_value();
        }
    );
    {
        auto ec = ep.send(publish_3_q2, as::use_future).get();
        BOOST_TEST(!ec);
        BOOST_TEST(!pub3_send);
    }

    // send publish_4
    bool pub4_send = false;
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(publish_4_q0, wp));
            pub4_send = true;
        }
    );
    {
        auto ec = ep.send(publish_4_q0, as::use_future).get();
        BOOST_TEST(!ec);
        BOOST_TEST(pub4_send);
    }

    // set send publish_3 checker again
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(publish_3_q2, wp));
            pub3_send = true;
            p.set_value();
        }
    );

    // recv puback2
    {
        auto pv = ep.recv(as::use_future).get();
        BOOST_TEST(am::packet_compare(puback2, pv));
    }

    f.get();

    // recv pubrec3
    {
        auto pv = ep.recv(as::use_future).get();
        BOOST_TEST(am::packet_compare(pubrec3, pv));
    }

    // send pubrel3
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(pubrel3, wp));
        }
    );
    {
        auto ec = ep.send(pubrel3, as::use_future).get();
        BOOST_TEST(!ec);
    }

    // recv pubcomp3
    {
        auto pv = ep.recv(as::use_future).get();
        BOOST_TEST(am::packet_compare(pubcomp3, pv));
    }
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

    auto connect = am::v5::connect_packet{
        true,   // clean_start
        0x1234, // keep_alive
        am::allocate_buffer("cid1"),
        am::nullopt, // will
        am::allocate_buffer("user1"),
        am::allocate_buffer("pass1"),
        am::properties{
            am::property::receive_maximum{2}
        }
    };

    auto connack = am::v5::connack_packet{
        false,   // session_present
        am::connect_reason_code::success,
        am::properties{}
    };

    // internal
    auto disconnect = am::v5::disconnect_packet{
        am::disconnect_reason_code::receive_maximum_exceeded
    };

    auto close = am::make_error(am::errc::network_reset, "pseudo close");

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

    auto publish_1_q1 = am::v5::publish_packet(
        0x1, // packet_id
        am::allocate_buffer("topic1"),
        am::allocate_buffer("payload1"),
        am::qos::at_least_once,
        am::properties{}
    );

    auto publish_2_q1 = am::v5::publish_packet(
        0x2, // packet_id
        am::allocate_buffer("topic1"),
        am::allocate_buffer("payload1"),
        am::qos::at_least_once,
        am::properties{}
    );

    auto publish_3_q0 = am::v5::publish_packet(
        0x0, // packet_id
        am::allocate_buffer("topic1"),
        am::allocate_buffer("payload1"),
        am::qos::at_most_once,
        am::properties{}
    );

    auto publish_4_q2 = am::v5::publish_packet(
        0x3, // packet_id
        am::allocate_buffer("topic1"),
        am::allocate_buffer("payload1"),
        am::qos::exactly_once,
        am::properties{}
    );

    auto publish_5_q1 = am::v5::publish_packet(
        0x5, // packet_id
        am::allocate_buffer("topic1"),
        am::allocate_buffer("payload1"),
        am::qos::at_least_once,
        am::properties{}
    );

    auto publish_6_q1 = am::v5::publish_packet(
        0x6, // packet_id
        am::allocate_buffer("topic1"),
        am::allocate_buffer("payload1"),
        am::qos::at_least_once,
        am::properties{}
    );

    auto puback2 = am::v5::puback_packet(
        0x1 // packet_id
    );

    auto pubrec3 = am::v5::pubrec_packet(
        0x3 // packet_id
    );

    auto pubrel3 = am::v5::pubrel_packet(
        0x3 // packet_id
    );

    auto pubcomp3 = am::v5::pubcomp_packet(
        0x3 // packet_id
    );

    ep.next_layer().set_recv_packets(
        {
            // receive packets
            publish_1_q1,
            publish_2_q1,
            publish_3_q0,
            publish_4_q2,
            close,
        }
    );

    // recv publish1
    {
        auto pv = ep.recv(as::use_future).get();
        BOOST_TEST(am::packet_compare(publish_1_q1, pv));
    }
    // recv publish2
    {
        auto pv = ep.recv(as::use_future).get();
        BOOST_TEST(am::packet_compare(publish_2_q1, pv));
    }
    // recv publish3
    {
        auto pv = ep.recv(as::use_future).get();
        BOOST_TEST(am::packet_compare(publish_3_q0, pv));
    }

    bool close_called = false;
    ep.next_layer().set_close_checker(
        [&] { close_called = true; }
    );
    // internal auto send disconnect
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(!close_called);
            BOOST_TEST(am::packet_compare(disconnect, wp));
        }
    );
    // recv publish4
    {
        auto pv = ep.recv(as::use_future).get();
        BOOST_TEST(pv.get_if<am::system_error>() != nullptr);
        BOOST_TEST(pv.get_if<am::system_error>()->code() == am::errc::bad_message);
    }
    BOOST_TEST(close_called);
    // recv close
    {
        auto pv = ep.recv(as::use_future).get();
        BOOST_TEST(pv.get_if<am::system_error>() != nullptr);
    }


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

    ep.next_layer().set_recv_packets(
        {
            // receive packets
            publish_1_q1,
            publish_4_q2,
            pubrel3,
            publish_2_q1,
            publish_3_q0,
            publish_5_q1,
            publish_6_q1,
            close,
        }
    );

    // recv publish1
    {
        auto pv = ep.recv(as::use_future).get();
        BOOST_TEST(am::packet_compare(publish_1_q1, pv));
    }
    // recv publish4
    {
        auto pv = ep.recv(as::use_future).get();
        BOOST_TEST(am::packet_compare(publish_4_q2, pv));
    }
    // send puback2
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(puback2, wp));
        }
    );
    {
        auto ec = ep.send(puback2, as::use_future).get();
        BOOST_TEST(!ec);
    }
    // send pubrec3
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(pubrec3, wp));
        }
    );
    {
        auto ec = ep.send(pubrec3, as::use_future).get();
        BOOST_TEST(!ec);
    }
    // recv pubrel3
    {
        auto pv = ep.recv(as::use_future).get();
        BOOST_TEST(am::packet_compare(pubrel3, pv));
    }
    // send pubcomp3
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(pubcomp3, wp));
        }
    );
    {
        auto ec = ep.send(pubcomp3, as::use_future).get();
        BOOST_TEST(!ec);
    }
    // recv publish2
    {
        auto pv = ep.recv(as::use_future).get();
        BOOST_TEST(am::packet_compare(publish_2_q1, pv));
    }
    // recv publish3
    {
        auto pv = ep.recv(as::use_future).get();
        BOOST_TEST(am::packet_compare(publish_3_q0, pv));
    }
    // recv publish5
    {
        auto pv = ep.recv(as::use_future).get();
        BOOST_TEST(am::packet_compare(publish_5_q1, pv));
    }

    close_called = false;
    ep.next_layer().set_close_checker(
        [&] { close_called = true; }
    );
    // internal auto send disconnect
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(!close_called);
            BOOST_TEST(am::packet_compare(disconnect, wp));
        }
    );
    // recv publish6
    {
        auto pv = ep.recv(as::use_future).get();
        BOOST_TEST(pv.get_if<am::system_error>() != nullptr);
        BOOST_TEST(pv.get_if<am::system_error>()->code() == am::errc::bad_message);
    }
    BOOST_TEST(close_called);
    ep.close(as::use_future).get();
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

    am::endpoint<async_mqtt::role::server, async_mqtt::stub_socket> ep1{
        version,
        // for stub_socket args
        version,
        ioc
    };
    am::endpoint<async_mqtt::role::server, async_mqtt::stub_socket> ep2{
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
        am::properties{}
    };

    auto connack = am::v5::connack_packet{
        false,   // session_present
        am::connect_reason_code::success,
        am::properties{
            am::property::receive_maximum{2}
        }
    };

    // internal
    auto disconnect = am::v5::disconnect_packet{
        am::disconnect_reason_code::receive_maximum_exceeded
    };

    auto close = am::make_error(am::errc::network_reset, "pseudo close");

    ep1.next_layer().set_recv_packets(
        {
            // receive packets
            connect,
        }
    );

    // recv connect
    {
        auto pv = ep1.recv(as::use_future).get();
        BOOST_TEST(am::packet_compare(connect, pv));
    }

    // send connack
    ep1.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(connack, wp));
        }
    );
    {
        auto ec = ep1.send(connack, as::use_future).get();
        BOOST_TEST(!ec);
    }

    auto publish_1_q1 = am::v5::publish_packet(
        0x1, // packet_id
        am::allocate_buffer("topic1"),
        am::allocate_buffer("payload1"),
        am::qos::at_least_once,
        am::properties{}
    );

    auto publish_2_q1 = am::v5::publish_packet(
        0x2, // packet_id
        am::allocate_buffer("topic1"),
        am::allocate_buffer("payload1"),
        am::qos::at_least_once,
        am::properties{}
    );

    auto publish_3_q0 = am::v5::publish_packet(
        0x0, // packet_id
        am::allocate_buffer("topic1"),
        am::allocate_buffer("payload1"),
        am::qos::at_most_once,
        am::properties{}
    );

    auto publish_4_q2 = am::v5::publish_packet(
        0x3, // packet_id
        am::allocate_buffer("topic1"),
        am::allocate_buffer("payload1"),
        am::qos::exactly_once,
        am::properties{}
    );

    auto publish_5_q1 = am::v5::publish_packet(
        0x5, // packet_id
        am::allocate_buffer("topic1"),
        am::allocate_buffer("payload1"),
        am::qos::at_least_once,
        am::properties{}
    );

    auto publish_6_q1 = am::v5::publish_packet(
        0x6, // packet_id
        am::allocate_buffer("topic1"),
        am::allocate_buffer("payload1"),
        am::qos::at_least_once,
        am::properties{}
    );

    auto puback2 = am::v5::puback_packet(
        0x1 // packet_id
    );

    auto pubrec3 = am::v5::pubrec_packet(
        0x3 // packet_id
    );

    auto pubrel3 = am::v5::pubrel_packet(
        0x3 // packet_id
    );

    auto pubcomp3 = am::v5::pubcomp_packet(
        0x3 // packet_id
    );

    ep1.next_layer().set_recv_packets(
        {
            // receive packets
            publish_1_q1,
            publish_2_q1,
            publish_3_q0,
            publish_4_q2,
            close,
        }
    );

    // recv publish1
    {
        auto pv = ep1.recv(as::use_future).get();
        BOOST_TEST(am::packet_compare(publish_1_q1, pv));
    }
    // recv publish2
    {
        auto pv = ep1.recv(as::use_future).get();
        BOOST_TEST(am::packet_compare(publish_2_q1, pv));
    }
    // recv publish3
    {
        auto pv = ep1.recv(as::use_future).get();
        BOOST_TEST(am::packet_compare(publish_3_q0, pv));
    }

    bool close_called = false;
    ep1.next_layer().set_close_checker(
        [&] { close_called = true; }
    );
    // internal auto send disconnect
    ep1.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(!close_called);
            BOOST_TEST(am::packet_compare(disconnect, wp));
        }
    );
    // recv publish4
    {
        auto pv = ep1.recv(as::use_future).get();
        BOOST_TEST(pv.get_if<am::system_error>() != nullptr);
        BOOST_TEST(pv.get_if<am::system_error>()->code() == am::errc::bad_message);
    }
    BOOST_TEST(close_called);
    // recv close
    {
        auto pv = ep1.recv(as::use_future).get();
        BOOST_TEST(pv.get_if<am::system_error>() != nullptr);
    }


    ep2.next_layer().set_recv_packets(
        {
            // receive packets
            connect,
        }
    );

    // recv connect
    {
        auto pv = ep2.recv(as::use_future).get();
        BOOST_TEST(am::packet_compare(connect, pv));
    }

    // send connack
    ep2.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(connack, wp));
        }
    );
    {
        auto ec = ep2.send(connack, as::use_future).get();
        BOOST_TEST(!ec);
    }

    ep2.next_layer().set_recv_packets(
        {
            // receive packets
            publish_1_q1,
            publish_4_q2,
            pubrel3,
            publish_2_q1,
            publish_3_q0,
            publish_5_q1,
            publish_6_q1,
            close,
        }
    );

    // recv publish1
    {
        auto pv = ep2.recv(as::use_future).get();
        BOOST_TEST(am::packet_compare(publish_1_q1, pv));
    }
    // recv publish4
    {
        auto pv = ep2.recv(as::use_future).get();
        BOOST_TEST(am::packet_compare(publish_4_q2, pv));
    }
    // send puback2
    ep2.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(puback2, wp));
        }
    );
    {
        auto ec = ep2.send(puback2, as::use_future).get();
        BOOST_TEST(!ec);
    }
    // send pubrec3
    ep2.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(pubrec3, wp));
        }
    );
    {
        auto ec = ep2.send(pubrec3, as::use_future).get();
        BOOST_TEST(!ec);
    }
    // recv pubrel3
    {
        auto pv = ep2.recv(as::use_future).get();
        BOOST_TEST(am::packet_compare(pubrel3, pv));
    }
    // send pubcomp3
    ep2.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(pubcomp3, wp));
        }
    );
    {
        auto ec = ep2.send(pubcomp3, as::use_future).get();
        BOOST_TEST(!ec);
    }
    // recv publish2
    {
        auto pv = ep2.recv(as::use_future).get();
        BOOST_TEST(am::packet_compare(publish_2_q1, pv));
    }
    // recv publish3
    {
        auto pv = ep2.recv(as::use_future).get();
        BOOST_TEST(am::packet_compare(publish_3_q0, pv));
    }
    // recv publish5
    {
        auto pv = ep2.recv(as::use_future).get();
        BOOST_TEST(am::packet_compare(publish_5_q1, pv));
    }

    close_called = false;
    ep2.next_layer().set_close_checker(
        [&] { close_called = true; }
    );
    // internal auto send disconnect
    ep2.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(!close_called);
            BOOST_TEST(am::packet_compare(disconnect, wp));
        }
    );
    // recv publish6
    {
        auto pv = ep2.recv(as::use_future).get();
        BOOST_TEST(pv.get_if<am::system_error>() != nullptr);
        BOOST_TEST(pv.get_if<am::system_error>()->code() == am::errc::bad_message);
    }
    BOOST_TEST(close_called);
    ep1.close(as::use_future).get();
    ep2.close(as::use_future).get();
    guard.reset();
    th.join();
}

BOOST_AUTO_TEST_SUITE_END()
