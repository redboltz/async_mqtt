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

BOOST_AUTO_TEST_SUITE(ut_ep_store)

namespace am = async_mqtt;
namespace as = boost::asio;

// v3_1_1

BOOST_AUTO_TEST_CASE(v311_client) {
    auto version = am::protocol_version::v3_1_1;
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
        ioc,
    };

    auto connect = am::v3_1_1::connect_packet{
        false,   // clean_session
        0x1234, // keep_alive
        am::allocate_buffer("cid1"),
        am::nullopt, // will
        am::allocate_buffer("user1"),
        am::allocate_buffer("pass1")
    };

    auto connack_sp_false = am::v3_1_1::connack_packet{
        false,   // session_present
        am::connect_return_code::accepted
    };

    auto connack_sp_true = am::v3_1_1::connack_packet{
        true,   // session_present
        am::connect_return_code::accepted
    };

    ep.next_layer().set_recv_packets(
        {
            // receive packets
            connack_sp_false,
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

    // recv connack_sp_false
    {
        auto pv = ep.recv(as::use_future).get();
        BOOST_TEST(am::packet_compare(connack_sp_false, pv));
    }

    auto close = am::make_error(am::errc::network_reset, "pseudo close");

    auto publish0 = am::v3_1_1::publish_packet(
        0x0, // packet_id
        am::allocate_buffer("topic1"),
        am::allocate_buffer("payload0"),
        am::qos::at_most_once
    );

    auto pid_opt1 = ep.acquire_unique_packet_id(as::use_future).get();
    BOOST_TEST(pid_opt1.has_value());
    auto publish1 = am::v3_1_1::publish_packet(
        *pid_opt1,
        am::allocate_buffer("topic1"),
        am::allocate_buffer("payload1"),
        am::qos::at_least_once
    );

    auto publish1dup{publish1};
    publish1dup.set_dup(true);

    auto pid_opt2 = ep.acquire_unique_packet_id(as::use_future).get();
    BOOST_TEST(pid_opt2.has_value());
    auto publish2 = am::v3_1_1::publish_packet(
        *pid_opt2,
        am::allocate_buffer("topic1"),
        am::allocate_buffer("payload2"),
        am::qos::exactly_once
    );

    auto publish2dup{publish2};
    publish2dup.set_dup(true);

    auto pubrec2 = am::v3_1_1::pubrec_packet(
        *pid_opt2
    );

    auto pubrel2 = am::v3_1_1::pubrel_packet(
        *pid_opt2
    );

    auto pid_opt3 = ep.acquire_unique_packet_id(as::use_future).get();
    BOOST_TEST(pid_opt3.has_value());
    auto pubrel3 = am::v3_1_1::pubrel_packet(
        *pid_opt3
    );

    ep.next_layer().set_recv_packets(
        {
            // receive packets
            close,
            connack_sp_true,
            pubrec2,
            close,
            connack_sp_true,
            close,
            connack_sp_false,
        }
    );

    // send publish0
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(publish0, wp));
        }
    );
    {
        auto ec = ep.send(publish0, as::use_future).get();
        BOOST_TEST(!ec);
    }

    // send publish1
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(publish1, wp));
        }
    );
    {
        auto ec = ep.send(publish1, as::use_future).get();
        BOOST_TEST(!ec);
    }

    // send publish2
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(publish2, wp));
        }
    );
    {
        auto ec = ep.send(publish2, as::use_future).get();
        BOOST_TEST(!ec);
    }

    // recv close
    {
        auto pv = ep.recv(as::use_future).get();
        BOOST_TEST(pv.get_if<am::system_error>() != nullptr);
    }

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

    // recv connack_sp_true
    std::size_t index = 0;
    std::promise<void> p;
    auto f = p.get_future();
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            switch (index++) {
            case 0:
                BOOST_TEST(am::packet_compare(publish1dup, wp));
                break;
            case 1:
                BOOST_TEST(am::packet_compare(publish2dup, wp));
                p.set_value();
                break;
            default:
                BOOST_TEST(false);
                break;
            }
        }
    );
    {
        auto pv = ep.recv(as::use_future).get();
        BOOST_TEST(am::packet_compare(connack_sp_true, pv));
    }
    f.get();
    BOOST_TEST(index == 2);

    // recv pubrec2
    {
        auto pv = ep.recv(as::use_future).get();
        BOOST_TEST(am::packet_compare(pubrec2, pv));
    }

    // send pubrel2
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(pubrel2, wp));
        }
    );
    {
        auto ec = ep.send(pubrel2, as::use_future).get();
        BOOST_TEST(!ec);
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

    // recv close
    {
        auto pv = ep.recv(as::use_future).get();
        BOOST_TEST(pv.get_if<am::system_error>() != nullptr);
    }

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

    // recv connack_sp_true
    index = 0;
    p = std::promise<void>();
    f = p.get_future();
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            switch (index++) {
            case 0:
                BOOST_TEST(am::packet_compare(publish1dup, wp));
                break;
            case 1:
                BOOST_TEST(am::packet_compare(pubrel2, wp));
                break;
            case 2:
                BOOST_TEST(am::packet_compare(pubrel3, wp));
                p.set_value();
                break;
            default:
                BOOST_TEST(false);
                break;
            }
        }
    );
    {
        auto pv = ep.recv(as::use_future).get();
        BOOST_TEST(am::packet_compare(connack_sp_true, pv));
    }
    f.get();
    BOOST_TEST(index == 3);

    // recv close
    {
        auto pv = ep.recv(as::use_future).get();
        BOOST_TEST(pv.get_if<am::system_error>() != nullptr);
    }

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

    // recv connack_sp_false
    index = 0;
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto pv = ep.recv(as::use_future).get();
        BOOST_TEST(am::packet_compare(connack_sp_false, pv));
    }
    ep.close(as::use_future).get();
    guard.reset();
    th.join();
}

BOOST_AUTO_TEST_CASE(v311_server) {
    auto version = am::protocol_version::v3_1_1;
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
    am::endpoint<async_mqtt::role::server, async_mqtt::stub_socket> ep3{
        version,
        // for stub_socket args
        version,
        ioc
    };
    am::endpoint<async_mqtt::role::server, async_mqtt::stub_socket> ep4{
        version,
        // for stub_socket args
        version,
        ioc
    };

    auto connect_no_clean = am::v3_1_1::connect_packet{
        false,   // clean_session
        0x1234, // keep_alive
        am::allocate_buffer("cid1"),
        am::nullopt, // will
        am::allocate_buffer("user1"),
        am::allocate_buffer("pass1")
    };

    auto connect_clean = am::v3_1_1::connect_packet{
        true,   // clean_session
        0x1234, // keep_alive
        am::allocate_buffer("cid1"),
        am::nullopt, // will
        am::allocate_buffer("user1"),
        am::allocate_buffer("pass1")
    };

    auto connack_sp_false = am::v3_1_1::connack_packet{
        false,   // session_present
        am::connect_return_code::accepted
    };

    auto connack_sp_true = am::v3_1_1::connack_packet{
        true,   // session_present
        am::connect_return_code::accepted
    };

    auto close = am::make_error(am::errc::network_reset, "pseudo close");

    ep1.next_layer().set_recv_packets(
        {
            // receive packets
            connect_no_clean,
        }
    );

    // recv connect_no_clean
    {
        auto pv = ep1.recv(as::use_future).get();
        BOOST_TEST(am::packet_compare(connect_no_clean, pv));
    }

    // send connack_sp_false
    ep1.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(connack_sp_false, wp));
        }
    );
    {
        auto ec = ep1.send(connack_sp_false, as::use_future).get();
        BOOST_TEST(!ec);
    }

    auto publish0 = am::v3_1_1::publish_packet(
        0x0, // packet_id
        am::allocate_buffer("topic1"),
        am::allocate_buffer("payload0"),
        am::qos::at_most_once
    );

    auto pid_opt1 = ep1.acquire_unique_packet_id(as::use_future).get();
    BOOST_TEST(pid_opt1.has_value());
    auto publish1 = am::v3_1_1::publish_packet(
        *pid_opt1,
        am::allocate_buffer("topic1"),
        am::allocate_buffer("payload1"),
        am::qos::at_least_once
    );

    auto publish1dup{publish1};
    publish1dup.set_dup(true);

    auto pid_opt2 = ep1.acquire_unique_packet_id(as::use_future).get();
    BOOST_TEST(pid_opt2.has_value());
    auto publish2 = am::v3_1_1::publish_packet(
        *pid_opt2,
        am::allocate_buffer("topic1"),
        am::allocate_buffer("payload2"),
        am::qos::exactly_once
    );

    auto publish2dup{publish2};
    publish2dup.set_dup(true);

    auto pubrec2 = am::v3_1_1::pubrec_packet(
        *pid_opt2
    );

    auto pubrel2 = am::v3_1_1::pubrel_packet(
        *pid_opt2
    );

    auto pid_opt3 = ep1.acquire_unique_packet_id(as::use_future).get();
    BOOST_TEST(pid_opt3.has_value());
    auto pubrel3 = am::v3_1_1::pubrel_packet(
        *pid_opt3
    );

    ep1.next_layer().set_recv_packets(
        {
            // receive packets
            close,
        }
    );
    ep2.next_layer().set_recv_packets(
        {
            // receive packets
            connect_no_clean,
            pubrec2,
            close,
        }
    );
    ep3.next_layer().set_recv_packets(
        {
            // receive packets
            connect_no_clean,
            close,
        }
    );
    ep4.next_layer().set_recv_packets(
        {
            // receive packets
            connect_clean,
        }
    );

    // send publish0
    ep1.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(publish0, wp));
        }
    );
    {
        auto ec = ep1.send(publish0, as::use_future).get();
        BOOST_TEST(!ec);
    }

    // send publish1
    ep1.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(publish1, wp));
        }
    );
    {
        auto ec = ep1.send(publish1, as::use_future).get();
        BOOST_TEST(!ec);
    }

    // send publish2
    ep1.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(publish2, wp));
        }
    );
    {
        auto ec = ep1.send(publish2, as::use_future).get();
        BOOST_TEST(!ec);
    }

    // recv close
    {
        auto pv = ep1.recv(as::use_future).get();
        BOOST_TEST(pv.get_if<am::system_error>() != nullptr);
    }

    // recv connect_no_clean
    {
        auto pv = ep2.recv(as::use_future).get();
        BOOST_TEST(am::packet_compare(connect_no_clean, pv));
    }

    // get_stored and restore next endpoint
    {
        auto pvs = ep1.get_stored_packets(as::use_future).get();
        ep2.restore_packets(am::force_move(pvs), as::use_future);
    }

    // send connack_sp_true
    std::size_t index = 0;
    std::promise<void> p;
    auto f = p.get_future();
    ep2.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            switch (index++) {
            case 0:
                BOOST_TEST(am::packet_compare(connack_sp_true, wp));
                break;
            case 1:
                BOOST_TEST(am::packet_compare(publish1dup, wp));
                break;
            case 2:
                BOOST_TEST(am::packet_compare(publish2dup, wp));
                p.set_value();
                break;
            default:
                BOOST_TEST(false);
                break;
            }
        }
    );
    {
        auto ec = ep2.send(connack_sp_true, as::use_future).get();
        BOOST_TEST(!ec);
    }
    f.get();
    BOOST_TEST(index == 3);
    {
        auto pid_opt = ep2.acquire_unique_packet_id(as::use_future).get();
        BOOST_TEST(pid_opt.has_value());
        BOOST_TEST(*pid_opt == 3); // 1 and 2 are used by publish(dup)
    }

    // recv pubrec2
    {
        auto pv = ep2.recv(as::use_future).get();
        BOOST_TEST(am::packet_compare(pubrec2, pv));
    }

    // send pubrel2
    ep2.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(pubrel2, wp));
        }
    );
    {
        auto ec = ep2.send(pubrel2, as::use_future).get();
        BOOST_TEST(!ec);
    }

    // send pubrel3
    ep2.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(pubrel3, wp));
        }
    );
    {
        auto ec = ep2.send(pubrel3, as::use_future).get();
        BOOST_TEST(!ec);
    }

    // recv close
    {
        auto pv = ep2.recv(as::use_future).get();
        BOOST_TEST(pv.get_if<am::system_error>() != nullptr);
    }

    // recv connect_no_clean
    {
        auto pv = ep3.recv(as::use_future).get();
        BOOST_TEST(am::packet_compare(connect_no_clean, pv));
    }

    // get_stored and restore next endpoint
    {
        auto pvs = ep2.get_stored_packets(as::use_future).get();
        ep3.restore_packets(am::force_move(pvs), as::use_future);
    }

    // send connack_sp_true
    index = 0;
    p = std::promise<void>();
    f = p.get_future();
    ep3.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            switch (index++) {
            case 0:
                BOOST_TEST(am::packet_compare(connack_sp_true, wp));
                break;
            case 1:
                BOOST_TEST(am::packet_compare(publish1dup, wp));
                break;
            case 2:
                BOOST_TEST(am::packet_compare(pubrel2, wp));
                break;
            case 3:
                BOOST_TEST(am::packet_compare(pubrel3, wp));
                p.set_value();
                break;
            default:
                BOOST_TEST(false);
                break;
            }
        }
    );
    {
        auto ec = ep3.send(connack_sp_true, as::use_future).get();
        BOOST_TEST(!ec);
    }
    f.get();
    BOOST_TEST(index == 4);
    {
        auto pid_opt = ep3.acquire_unique_packet_id(as::use_future).get();
        BOOST_TEST(pid_opt.has_value());
        BOOST_TEST(*pid_opt == 4); // 1, 2 and 3 are used by publish(dup) and pubrel
    }

    // recv close
    {
        auto pv = ep3.recv(as::use_future).get();
        BOOST_TEST(pv.get_if<am::system_error>() != nullptr);
    }

    // recv connect_clean
    {
        auto pv = ep4.recv(as::use_future).get();
        BOOST_TEST(am::packet_compare(connect_clean, pv));
    }

    // no restore because clean_session is true

    // recv connack_sp_false
    index = 0;
    p = std::promise<void>();
    f = p.get_future();
    ep4.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            switch (index++) {
            case 0:
                BOOST_TEST(am::packet_compare(connack_sp_false, wp));
                p.set_value();
                break;
            default:
                BOOST_TEST(false);
                break;
            }
        }
    );
    {
        auto ec = ep4.send(connack_sp_false, as::use_future).get();
        BOOST_TEST(!ec);
    }
    f.get();
    BOOST_TEST(index == 1);
    ep1.close(as::use_future).get();
    ep2.close(as::use_future).get();
    ep3.close(as::use_future).get();
    ep4.close(as::use_future).get();
    guard.reset();
    th.join();
}

// v5

BOOST_AUTO_TEST_CASE(v5_client) {
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
        false,   // clean_session
        0x1234, // keep_alive
        am::allocate_buffer("cid1"),
        am::nullopt, // will
        am::allocate_buffer("user1"),
        am::allocate_buffer("pass1"),
        am::properties{
            am::property::session_expiry_interval{am::session_never_expire}
        }
    };

    auto connack_sp_false = am::v5::connack_packet{
        false,   // session_present
        am::connect_reason_code::success,
        am::properties{}
    };

    auto connack_sp_true = am::v5::connack_packet{
        true,   // session_present
        am::connect_reason_code::success,
        am::properties{}
    };

    auto close = am::make_error(am::errc::network_reset, "pseudo close");

    ep.next_layer().set_recv_packets(
        {
            // receive packets
            connack_sp_false,
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

    // recv connack_sp_false
    {
        auto pv = ep.recv(as::use_future).get();
        BOOST_TEST(am::packet_compare(connack_sp_false, pv));
    }

    auto publish0 = am::v5::publish_packet(
        0x0, // packet_id
        am::allocate_buffer("topic1"),
        am::allocate_buffer("payload0"),
        am::qos::at_most_once,
        am::properties{}
    );

    auto pid_opt1 = ep.acquire_unique_packet_id(as::use_future).get();
    BOOST_TEST(pid_opt1.has_value());
    auto publish1 = am::v5::publish_packet(
        *pid_opt1,
        am::allocate_buffer("topic1"),
        am::allocate_buffer("payload1"),
        am::qos::at_least_once,
        am::properties{}
    );

    auto publish1dup{publish1};
    publish1dup.set_dup(true);

    auto pid_opt2 = ep.acquire_unique_packet_id(as::use_future).get();
    BOOST_TEST(pid_opt2.has_value());
    auto publish2 = am::v5::publish_packet(
        *pid_opt2,
        am::allocate_buffer("topic1"),
        am::allocate_buffer("payload2"),
        am::qos::exactly_once,
        am::properties{}
    );

    auto publish2dup{publish2};
    publish2dup.set_dup(true);

    auto pubrec2 = am::v3_1_1::pubrec_packet(
        *pid_opt2
    );

    auto pubrel2 = am::v3_1_1::pubrel_packet(
        *pid_opt2
    );

    auto pid_opt3 = ep.acquire_unique_packet_id(as::use_future).get();
    BOOST_TEST(pid_opt3.has_value());
    auto pubrel3 = am::v3_1_1::pubrel_packet(
        *pid_opt3
    );

    ep.next_layer().set_recv_packets(
        {
            // receive packets
            close,
            connack_sp_true,
            pubrec2,
            close,
            connack_sp_true,
            close,
            connack_sp_false,
        }
    );

    // send publish0
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(publish0, wp));
        }
    );
    {
        auto ec = ep.send(publish0, as::use_future).get();
        BOOST_TEST(!ec);
    }

    // send publish1
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(publish1, wp));
        }
    );
    {
        auto ec = ep.send(publish1, as::use_future).get();
        BOOST_TEST(!ec);
    }

    // send publish2
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(publish2, wp));
        }
    );
    {
        auto ec = ep.send(publish2, as::use_future).get();
        BOOST_TEST(!ec);
    }

    // recv close
    {
        auto pv = ep.recv(as::use_future).get();
        BOOST_TEST(pv.get_if<am::system_error>() != nullptr);
    }

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

    // recv connack_sp_true
    std::size_t index = 0;
    std::promise<void> p;
    auto f = p.get_future();
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            switch (index++) {
            case 0:
                BOOST_TEST(am::packet_compare(publish1dup, wp));
                break;
            case 1:
                BOOST_TEST(am::packet_compare(publish2dup, wp));
                p.set_value();
                break;
            default:
                BOOST_TEST(false);
                break;
            }
        }
    );
    {
        auto pv = ep.recv(as::use_future).get();
        BOOST_TEST(am::packet_compare(connack_sp_true, pv));
    }
    f.get();
    BOOST_TEST(index == 2);

    // recv pubrec2
    {
        auto pv = ep.recv(as::use_future).get();
        BOOST_TEST(am::packet_compare(pubrec2, pv));
    }

    // send pubrel2
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(pubrel2, wp));
        }
    );
    {
        auto ec = ep.send(pubrel2, as::use_future).get();
        BOOST_TEST(!ec);
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

    // recv close
    {
        auto pv = ep.recv(as::use_future).get();
        BOOST_TEST(pv.get_if<am::system_error>() != nullptr);
    }

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

    // recv connack_sp_true
    index = 0;
    p = std::promise<void>();
    f = p.get_future();
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            switch (index++) {
            case 0:
                BOOST_TEST(am::packet_compare(publish1dup, wp));
                break;
            case 1:
                BOOST_TEST(am::packet_compare(pubrel2, wp));
                break;
            case 2:
                BOOST_TEST(am::packet_compare(pubrel3, wp));
                p.set_value();
                break;
            default:
                BOOST_TEST(false);
                break;
            }
        }
    );
    {
        auto pv = ep.recv(as::use_future).get();
        BOOST_TEST(am::packet_compare(connack_sp_true, pv));
    }
    f.get();
    BOOST_TEST(index == 3);

    // recv close
    {
        auto pv = ep.recv(as::use_future).get();
        BOOST_TEST(pv.get_if<am::system_error>() != nullptr);
    }

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

    // recv connack_sp_false
    index = 0;
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto pv = ep.recv(as::use_future).get();
        BOOST_TEST(am::packet_compare(connack_sp_false, pv));
    }
    ep.close(as::use_future).get();
    guard.reset();
    th.join();
}

BOOST_AUTO_TEST_CASE(v5_server) {
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
    am::endpoint<async_mqtt::role::server, async_mqtt::stub_socket> ep3{
        version,
        // for stub_socket args
        version,
        ioc
    };
    am::endpoint<async_mqtt::role::server, async_mqtt::stub_socket> ep4{
        version,
        // for stub_socket args
        version,
        ioc
    };

    auto connect_no_clean = am::v5::connect_packet{
        false,   // clean_session
        0x1234, // keep_alive
        am::allocate_buffer("cid1"),
        am::nullopt, // will
        am::allocate_buffer("user1"),
        am::allocate_buffer("pass1"),
        am::properties{
            am::property::session_expiry_interval{am::session_never_expire}
        }
    };

    auto connect_clean = am::v5::connect_packet{
        true,   // clean_session
        0x1234, // keep_alive
        am::allocate_buffer("cid1"),
        am::nullopt, // will
        am::allocate_buffer("user1"),
        am::allocate_buffer("pass1"),
        am::properties{}
    };

    auto connack_sp_false = am::v5::connack_packet{
        false,   // session_present
        am::connect_reason_code::success,
        am::properties{}
    };

    auto connack_sp_true = am::v5::connack_packet{
        true,   // session_present
        am::connect_reason_code::success,
        am::properties{}
    };

    auto close = am::make_error(am::errc::network_reset, "pseudo close");

    ep1.next_layer().set_recv_packets(
        {
            // receive packets
            connect_no_clean,
        }
    );

    // recv connect_no_clean
    {
        auto pv = ep1.recv(as::use_future).get();
        BOOST_TEST(am::packet_compare(connect_no_clean, pv));
    }

    // send connack_sp_false
    ep1.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(connack_sp_false, wp));
        }
    );
    {
        auto ec = ep1.send(connack_sp_false, as::use_future).get();
        BOOST_TEST(!ec);
    }

    auto publish0 = am::v5::publish_packet(
        0x0, // packet_id
        am::allocate_buffer("topic1"),
        am::allocate_buffer("payload0"),
        am::qos::at_most_once,
        am::properties{}
    );

    auto pid_opt1 = ep1.acquire_unique_packet_id(as::use_future).get();
    BOOST_TEST(pid_opt1.has_value());
    auto publish1 = am::v5::publish_packet(
        *pid_opt1,
        am::allocate_buffer("topic1"),
        am::allocate_buffer("payload1"),
        am::qos::at_least_once,
        am::properties{}
    );

    auto publish1dup{publish1};
    publish1dup.set_dup(true);

    auto pid_opt2 = ep1.acquire_unique_packet_id(as::use_future).get();
    BOOST_TEST(pid_opt2.has_value());
    auto publish2 = am::v5::publish_packet(
        *pid_opt2,
        am::allocate_buffer("topic1"),
        am::allocate_buffer("payload2"),
        am::qos::exactly_once,
        am::properties{}
    );

    auto publish2dup{publish2};
    publish2dup.set_dup(true);

    auto pubrec2 = am::v3_1_1::pubrec_packet(
        *pid_opt2
    );

    auto pubrel2 = am::v3_1_1::pubrel_packet(
        *pid_opt2
    );

    auto pid_opt3 = ep1.acquire_unique_packet_id(as::use_future).get();
    BOOST_TEST(pid_opt3.has_value());
    auto pubrel3 = am::v3_1_1::pubrel_packet(
        *pid_opt3
    );

    ep1.next_layer().set_recv_packets(
        {
            // receive packets
            close,
        }
    );
    ep2.next_layer().set_recv_packets(
        {
            // receive packets
            connect_no_clean,
            pubrec2,
            close,
        }
    );
    ep3.next_layer().set_recv_packets(
        {
            // receive packets
            connect_no_clean,
            close,
        }
    );
    ep4.next_layer().set_recv_packets(
        {
            // receive packets
            connect_clean,
        }
    );

    // send publish0
    ep1.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(publish0, wp));
        }
    );
    {
        auto ec = ep1.send(publish0, as::use_future).get();
        BOOST_TEST(!ec);
    }

    // send publish1
    ep1.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(publish1, wp));
        }
    );
    {
        auto ec = ep1.send(publish1, as::use_future).get();
        BOOST_TEST(!ec);
    }

    // send publish2
    ep1.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(publish2, wp));
        }
    );
    {
        auto ec = ep1.send(publish2, as::use_future).get();
        BOOST_TEST(!ec);
    }

    // recv close
    {
        auto pv = ep1.recv(as::use_future).get();
        BOOST_TEST(pv.get_if<am::system_error>() != nullptr);
    }

    // recv connect_no_clean
    {
        auto pv = ep2.recv(as::use_future).get();
        BOOST_TEST(am::packet_compare(connect_no_clean, pv));
    }

    // get_stored and restore next endpoint
    {
        auto pvs = ep1.get_stored_packets(as::use_future).get();
        ep2.restore_packets(am::force_move(pvs), as::use_future);
    }

    // send connack_sp_true
    std::size_t index = 0;
    std::promise<void> p;
    auto f = p.get_future();
    ep2.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            switch (index++) {
            case 0:
                BOOST_TEST(am::packet_compare(connack_sp_true, wp));
                break;
            case 1:
                BOOST_TEST(am::packet_compare(publish1dup, wp));
                break;
            case 2:
                BOOST_TEST(am::packet_compare(publish2dup, wp));
                p.set_value();
                break;
            default:
                BOOST_TEST(false);
                break;
            }
        }
    );
    {
        auto ec = ep2.send(connack_sp_true, as::use_future).get();
        BOOST_TEST(!ec);
    }
    f.get();
    BOOST_TEST(index == 3);

    {
        auto pid_opt = ep2.acquire_unique_packet_id(as::use_future).get();
        BOOST_TEST(pid_opt.has_value());
        BOOST_TEST(*pid_opt == 3); // 1 and 2 are used by publish(dup)
    }

    // recv pubrec2
    {
        auto pv = ep2.recv(as::use_future).get();
        BOOST_TEST(am::packet_compare(pubrec2, pv));
    }

    // send pubrel2
    ep2.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(pubrel2, wp));
        }
    );
    {
        auto ec = ep2.send(pubrel2, as::use_future).get();
        BOOST_TEST(!ec);
    }

    // send pubrel3
    ep2.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(pubrel3, wp));
        }
    );
    {
        auto ec = ep2.send(pubrel3, as::use_future).get();
        BOOST_TEST(!ec);
    }

    // recv close
    {
        auto pv = ep2.recv(as::use_future).get();
        BOOST_TEST(pv.get_if<am::system_error>() != nullptr);
    }

    // recv connect_no_clean
    {
        auto pv = ep3.recv(as::use_future).get();
        BOOST_TEST(am::packet_compare(connect_no_clean, pv));
    }

    // get_stored and restore next endpoint
    {
        auto pvs = ep2.get_stored_packets(as::use_future).get();
        ep3.restore_packets(am::force_move(pvs), as::use_future);
    }

    // send connack_sp_true
    index = 0;
    p = std::promise<void>();
    f = p.get_future();
    ep3.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            switch (index++) {
            case 0:
                BOOST_TEST(am::packet_compare(connack_sp_true, wp));
                break;
            case 1:
                BOOST_TEST(am::packet_compare(publish1dup, wp));
                break;
            case 2:
                BOOST_TEST(am::packet_compare(pubrel2, wp));
                break;
            case 3:
                BOOST_TEST(am::packet_compare(pubrel3, wp));
                p.set_value();
                break;
            default:
                BOOST_TEST(false);
                break;
            }
        }
    );
    {
        auto ec = ep3.send(connack_sp_true, as::use_future).get();
        BOOST_TEST(!ec);
    }
    f.get();
    BOOST_TEST(index == 4);
    {
        auto pid_opt = ep3.acquire_unique_packet_id(as::use_future).get();
        BOOST_TEST(pid_opt.has_value());
        BOOST_TEST(*pid_opt == 4); // 1, 2 and 3 are used by publish(dup) and pubrel
    }

    // recv close
    {
        auto pv = ep3.recv(as::use_future).get();
        BOOST_TEST(pv.get_if<am::system_error>() != nullptr);
    }

    // recv connect_clean
    {
        auto pv = ep4.recv(as::use_future).get();
        BOOST_TEST(am::packet_compare(connect_clean, pv));
    }

    // no restore because clean_session is true

    // recv connack_sp_false
    index = 0;
    p = std::promise<void>();
    f = p.get_future();
    ep4.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            switch (index++) {
            case 0:
                BOOST_TEST(am::packet_compare(connack_sp_false, wp));
                p.set_value();
                break;
            default:
                BOOST_TEST(false);
                break;
            }
        }
    );
    {
        auto ec = ep4.send(connack_sp_false, as::use_future).get();
        BOOST_TEST(!ec);
    }
    f.get();
    BOOST_TEST(index == 1);
    ep1.close(as::use_future).get();
    ep2.close(as::use_future).get();
    ep3.close(as::use_future).get();
    ep4.close(as::use_future).get();
    guard.reset();
    th.join();
}

BOOST_AUTO_TEST_CASE(v5_topic_alias) {
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
        false,   // clean_session
        0x1234, // keep_alive
        am::allocate_buffer("cid1"),
        am::nullopt, // will
        am::allocate_buffer("user1"),
        am::allocate_buffer("pass1"),
        am::properties{
            am::property::session_expiry_interval{am::session_never_expire}
        }
    };

    auto connack_sp_false = am::v5::connack_packet{
        false,   // session_present
        am::connect_reason_code::success,
        am::properties{
            am::property::topic_alias_maximum{5}
        }
    };

    auto connack_sp_true = am::v5::connack_packet{
        true,   // session_present
        am::connect_reason_code::success,
        am::properties{
            am::property::topic_alias_maximum{5}
        }
    };

    auto close = am::make_error(am::errc::network_reset, "pseudo close");

    ep.next_layer().set_recv_packets(
        {
            // receive packets
            connack_sp_false,
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

    // recv connack_sp_false
    {
        auto pv = ep.recv(as::use_future).get();
        BOOST_TEST(am::packet_compare(connack_sp_false, pv));
    }

    auto publish0 = am::v5::publish_packet(
        0x0, // packet_id
        am::allocate_buffer("topic1"),
        am::allocate_buffer("payload0"),
        am::qos::at_most_once,
        am::properties{}
    );

    auto pid_opt1 = ep.acquire_unique_packet_id(as::use_future).get();
    BOOST_TEST(pid_opt1.has_value());
    auto publish1 = am::v5::publish_packet(
        *pid_opt1,
        am::allocate_buffer("topic1"),
        am::allocate_buffer("payload1"),
        am::qos::at_least_once,
        am::properties{
            am::property::topic_alias{1}
        }
    );

    auto publish1no_ta = am::v5::publish_packet(
        *pid_opt1,
        am::allocate_buffer("topic1"),
        am::allocate_buffer("payload1"),
        am::qos::at_least_once,
        am::properties{}
    );

    auto publish1no_ta_dup{publish1no_ta};
    publish1no_ta_dup.set_dup(true);

    auto pid_opt2 = ep.acquire_unique_packet_id(as::use_future).get();
    BOOST_TEST(pid_opt2.has_value());
    auto publish2 = am::v5::publish_packet(
        *pid_opt2,
        am::buffer{},
        am::allocate_buffer("payload2"),
        am::qos::exactly_once,
        am::properties{
            am::property::topic_alias{1}
        }
    );

    auto publish2no_ta = am::v5::publish_packet(
        *pid_opt2,
        am::allocate_buffer("topic1"),
        am::allocate_buffer("payload2"),
        am::qos::exactly_once,
        am::properties{}
    );

    auto publish2no_ta_dup{publish2no_ta};
    publish2no_ta_dup.set_dup(true);

    auto pubrec2 = am::v3_1_1::pubrec_packet(
        *pid_opt2
    );

    auto pubrel2 = am::v3_1_1::pubrel_packet(
        *pid_opt2
    );

    auto pid_opt3 = ep.acquire_unique_packet_id(as::use_future).get();
    BOOST_TEST(pid_opt3.has_value());
    auto pubrel3 = am::v3_1_1::pubrel_packet(
        *pid_opt3
    );

    ep.next_layer().set_recv_packets(
        {
            // receive packets
            close,
            connack_sp_true,
            pubrec2,
            close,
            connack_sp_true,
            close,
            connack_sp_false,
        }
    );

    // send publish0
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(publish0, wp));
        }
    );
    {
        auto ec = ep.send(publish0, as::use_future).get();
        BOOST_TEST(!ec);
    }

    // send publish1
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(publish1, wp));
        }
    );
    {
        auto ec = ep.send(publish1, as::use_future).get();
        BOOST_TEST(!ec);
    }

    // send publish2
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(publish2, wp));
        }
    );
    {
        auto ec = ep.send(publish2, as::use_future).get();
        BOOST_TEST(!ec);
    }

    // recv close
    {
        auto pv = ep.recv(as::use_future).get();
        BOOST_TEST(pv.get_if<am::system_error>() != nullptr);
    }

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

    // recv connack_sp_true
    std::size_t index = 0;
    std::promise<void> p;
    auto f = p.get_future();
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            switch (index++) {
            case 0:
                BOOST_TEST(am::packet_compare(publish1no_ta_dup, wp));
                break;
            case 1:
                BOOST_TEST(am::packet_compare(publish2no_ta_dup, wp));
                p.set_value();
                break;
            default:
                BOOST_TEST(false);
                break;
            }
        }
    );
    {
        auto pv = ep.recv(as::use_future).get();
        BOOST_TEST(am::packet_compare(connack_sp_true, pv));
    }
    f.get();
    BOOST_TEST(index == 2);

    // recv pubrec2
    {
        auto pv = ep.recv(as::use_future).get();
        BOOST_TEST(am::packet_compare(pubrec2, pv));
    }

    // send pubrel2
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(pubrel2, wp));
        }
    );
    {
        auto ec = ep.send(pubrel2, as::use_future).get();
        BOOST_TEST(!ec);
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

    // recv close
    {
        auto pv = ep.recv(as::use_future).get();
        BOOST_TEST(pv.get_if<am::system_error>() != nullptr);
    }

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

    // recv connack_sp_true
    index = 0;
    p = std::promise<void>();
    f = p.get_future();
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            switch (index++) {
            case 0:
                BOOST_TEST(am::packet_compare(publish1no_ta_dup, wp));
                break;
            case 1:
                BOOST_TEST(am::packet_compare(pubrel2, wp));
                break;
            case 2:
                BOOST_TEST(am::packet_compare(pubrel3, wp));
                p.set_value();
                break;
            default:
                BOOST_TEST(false);
                break;
            }
        }
    );
    {
        auto pv = ep.recv(as::use_future).get();
        BOOST_TEST(am::packet_compare(connack_sp_true, pv));
    }
    f.get();
    BOOST_TEST(index == 3);

    // recv close
    {
        auto pv = ep.recv(as::use_future).get();
        BOOST_TEST(pv.get_if<am::system_error>() != nullptr);
    }

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

    // recv connack_sp_false
    index = 0;
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto pv = ep.recv(as::use_future).get();
        BOOST_TEST(am::packet_compare(connack_sp_false, pv));
    }
    ep.close(as::use_future).get();
    guard.reset();
    th.join();
}

BOOST_AUTO_TEST_SUITE_END()
