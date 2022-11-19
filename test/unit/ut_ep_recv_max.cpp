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

BOOST_AUTO_TEST_CASE(client) {
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

    auto puback2 = am::v5::puback_packet(
        0x2 // packet_id
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

    ep.stream().next_layer().set_recv_packets(
        {
            // receive packets
            connack,
            puback2,
            pubrec3,
            pubcomp3,
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
    ep.stream().next_layer().set_write_packet_checker(
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
    ep.stream().next_layer().set_write_packet_checker(
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
    ep.stream().next_layer().set_write_packet_checker(
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
    ep.stream().next_layer().set_write_packet_checker(
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

    guard.reset();
    th.join();
}

BOOST_AUTO_TEST_CASE(server) {
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

    ep.stream().next_layer().set_recv_packets(
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
    ep.stream().next_layer().set_write_packet_checker(
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

    ep.stream().next_layer().set_recv_packets(
        {
            // receive packets
            puback2,
            pubrec3,
            pubcomp3,
        }
    );

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
    ep.stream().next_layer().set_write_packet_checker(
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
    ep.stream().next_layer().set_write_packet_checker(
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
    ep.stream().next_layer().set_write_packet_checker(
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
    ep.stream().next_layer().set_write_packet_checker(
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

    guard.reset();
    th.join();
}

BOOST_AUTO_TEST_SUITE_END()
