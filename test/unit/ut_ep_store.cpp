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

    auto ep = am::endpoint<async_mqtt::role::client, async_mqtt::stub_socket>{
        version,
        // for stub_socket args
        version,
        ioc.get_executor()
    };
    ep.set_bulk_write(true);
    auto connect = am::v3_1_1::connect_packet{
        false,   // clean_session
        0x1234, // keep_alive
        "cid1",
        std::nullopt, // will
        "user1",
        "pass1"
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
            {connack_sp_false},
        }
    );

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

    // recv connack_sp_false
    {
        auto [ec, pv] = ep.async_recv(as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
        BOOST_TEST(connack_sp_false == pv);
    }

    auto publish0 = am::v3_1_1::publish_packet(
        0x0, // packet_id
        "topic1",
        "payload0",
        am::qos::at_most_once
    );

    auto [ec1, pid1] = ep.async_acquire_unique_packet_id(as::as_tuple(as::use_future)).get();
    BOOST_TEST(!ec1);
    auto publish1 = am::v3_1_1::publish_packet(
        pid1,
        "topic1",
        "payload1",
        am::qos::at_least_once
    );

    auto publish1dup{publish1};
    publish1dup.set_dup(true);

    auto [ec2, pid2] = ep.async_acquire_unique_packet_id(as::as_tuple(as::use_future)).get();
    BOOST_TEST(!ec2);
    auto publish2 = am::v3_1_1::publish_packet(
        pid2,
        "topic1",
        "payload2",
        am::qos::exactly_once
    );

    auto publish2dup{publish2};
    publish2dup.set_dup(true);

    auto pubrec2 = am::v3_1_1::pubrec_packet(
        pid2
    );

    auto pubrel2 = am::v3_1_1::pubrel_packet(
        pid2
    );

    auto [ec3, pid3] = ep.async_acquire_unique_packet_id(as::as_tuple(as::use_future)).get();
    BOOST_TEST(!ec3);
    auto pubrel3 = am::v3_1_1::pubrel_packet(
        pid3
    );

    ep.next_layer().set_recv_packets(
        {
            // receive packets
            {am::errc::make_error_code(am::errc::connection_reset)},
            {connack_sp_true},
            {pubrec2},
            {am::errc::make_error_code(am::errc::connection_reset)},
            {connack_sp_true},
            {am::errc::make_error_code(am::errc::connection_reset)},
            {connack_sp_false},
        }
    );

    // send publish0
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(publish0 == wp);
        }
    );
    {
        auto [ec] = ep.async_send(publish0, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }

    // send publish1
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(publish1 == wp);
        }
    );
    {
        auto [ec] = ep.async_send(publish1, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }

    // send publish2
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(publish2 == wp);
        }
    );
    {
        auto [ec] = ep.async_send(publish2, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }

    // recv close
    {
        auto [ec, pv] = ep.async_recv(as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::errc::connection_reset);
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

    // recv connack_sp_true
    std::size_t index = 0;
    std::promise<void> p;
    auto f = p.get_future();
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            switch (index++) {
            case 0:
                BOOST_TEST(publish1dup == wp);
                break;
            case 1:
                BOOST_TEST(publish2dup == wp);
                p.set_value();
                break;
            default:
                BOOST_TEST(false);
                break;
            }
        }
    );
    {
        auto [ec, pv] = ep.async_recv(as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
        BOOST_TEST(connack_sp_true == pv);
    }
    f.get();
    BOOST_TEST(index == 2);

    // recv pubrec2
    {
        auto [ec, pv] = ep.async_recv(as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
        BOOST_TEST(pubrec2 == pv);
    }

    // send pubrel2
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(pubrel2 == wp);
        }
    );
    {
        auto [ec] = ep.async_send(pubrel2, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }

    // send pubrel3
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(pubrel3 == wp);
        }
    );
    {
        auto [ec] = ep.async_send(pubrel3, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }

    // recv close
    {
        auto [ec, pv] = ep.async_recv(as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::errc::connection_reset);
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

    // recv connack_sp_true
    index = 0;
    p = std::promise<void>();
    f = p.get_future();
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            switch (index++) {
            case 0:
                BOOST_TEST(publish1dup == wp);
                break;
            case 1:
                BOOST_TEST(pubrel2 == wp);
                break;
            case 2:
                BOOST_TEST(pubrel3 == wp);
                p.set_value();
                break;
            default:
                BOOST_TEST(false);
                break;
            }
        }
    );
    {
        auto [ec, pv] = ep.async_recv(as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
        BOOST_TEST(connack_sp_true == pv);
    }
    f.get();
    BOOST_TEST(index == 3);

    // recv close
    {
        auto [ec, pv] = ep.async_recv(as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::errc::connection_reset);
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

    // recv connack_sp_false
    index = 0;
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec, pv] = ep.async_recv(as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
        BOOST_TEST(connack_sp_false == pv);
    }
    ep.async_close(as::as_tuple(as::use_future)).get();
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

    auto ep1 = am::endpoint<async_mqtt::role::server, async_mqtt::stub_socket>{
        version,
        // for stub_socket args
        version,
        ioc.get_executor()
    };
    auto ep2 = am::endpoint<async_mqtt::role::server, async_mqtt::stub_socket>{
        version,
        // for stub_socket args
        version,
        ioc.get_executor()
    };
    auto ep3 = am::endpoint<async_mqtt::role::server, async_mqtt::stub_socket>{
        version,
        // for stub_socket args
        version,
        ioc.get_executor()
    };
    auto ep4 = am::endpoint<async_mqtt::role::server, async_mqtt::stub_socket>{
        version,
        // for stub_socket args
        version,
        ioc.get_executor()
    };

    auto connect_no_clean = am::v3_1_1::connect_packet{
        false,   // clean_session
        0x1234, // keep_alive
        "cid1",
        std::nullopt, // will
        "user1",
        "pass1"
    };

    auto connect_clean = am::v3_1_1::connect_packet{
        true,   // clean_session
        0x1234, // keep_alive
        "cid1",
        std::nullopt, // will
        "user1",
        "pass1"
    };

    auto connack_sp_false = am::v3_1_1::connack_packet{
        false,   // session_present
        am::connect_return_code::accepted
    };

    auto connack_sp_true = am::v3_1_1::connack_packet{
        true,   // session_present
        am::connect_return_code::accepted
    };

    ep1.next_layer().set_recv_packets(
        {
            // receive packets
            {connect_no_clean},
        }
    );

    // recv connect_no_clean
    {
        auto [ec, pv] = ep1.async_recv(as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
        BOOST_TEST(connect_no_clean == pv);
    }

    // send connack_sp_false
    ep1.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(connack_sp_false == wp);
        }
    );
    {
        auto [ec] = ep1.async_send(connack_sp_false, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }

    auto publish0 = am::v3_1_1::publish_packet(
        0x0, // packet_id
        "topic1",
        "payload0",
        am::qos::at_most_once
    );

    auto [ec1, pid1] = ep1.async_acquire_unique_packet_id(as::as_tuple(as::use_future)).get();
    BOOST_TEST(!ec1);
    auto publish1 = am::v3_1_1::publish_packet(
        pid1,
        "topic1",
        "payload1",
        am::qos::at_least_once
    );

    auto publish1dup{publish1};
    publish1dup.set_dup(true);

    auto [ec2, pid2] = ep1.async_acquire_unique_packet_id(as::as_tuple(as::use_future)).get();
    BOOST_TEST(!ec2);
    auto publish2 = am::v3_1_1::publish_packet(
        pid2,
        "topic1",
        "payload2",
        am::qos::exactly_once
    );

    auto publish2dup{publish2};
    publish2dup.set_dup(true);

    auto pubrec2 = am::v3_1_1::pubrec_packet(
        pid2
    );

    auto pubrel2 = am::v3_1_1::pubrel_packet(
        pid2
    );

    auto [ec3, pid3] = ep1.async_acquire_unique_packet_id(as::as_tuple(as::use_future)).get();
    BOOST_TEST(!ec3);
    auto pubrel3 = am::v3_1_1::pubrel_packet(
        pid3
    );

    ep1.next_layer().set_recv_packets(
        {
            // receive packets
            {am::errc::make_error_code(am::errc::connection_reset)},
        }
    );
    ep2.next_layer().set_recv_packets(
        {
            // receive packets
            {connect_no_clean},
            {pubrec2},
            {am::errc::make_error_code(am::errc::connection_reset)},
        }
    );
    ep3.next_layer().set_recv_packets(
        {
            // receive packets
            {connect_no_clean},
            {am::errc::make_error_code(am::errc::connection_reset)},
        }
    );
    ep4.next_layer().set_recv_packets(
        {
            // receive packets
            {connect_clean},
        }
    );

    // send publish0
    ep1.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(publish0 == wp);
        }
    );
    {
        auto [ec] = ep1.async_send(publish0, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }

    // send publish1
    ep1.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(publish1 == wp);
        }
    );
    {
        auto [ec] = ep1.async_send(publish1, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }

    // send publish2
    ep1.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(publish2 == wp);
        }
    );
    {
        auto [ec] = ep1.async_send(publish2, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }

    // recv close
    {
        auto [ec, pv] = ep1.async_recv(as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::errc::connection_reset);
    }

    // recv connect_no_clean
    {
        auto [ec, pv] = ep2.async_recv(as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
        BOOST_TEST(connect_no_clean == pv);
    }

    // get_stored and restore next endpoint
    {
        auto [ec, pvs] = ep1.async_get_stored_packets(as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
        ep2.async_restore_packets(am::force_move(pvs), as::as_tuple(as::use_future));
    }

    // send connack_sp_true
    std::size_t index = 0;
    std::promise<void> p;
    auto f = p.get_future();
    ep2.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            switch (index++) {
            case 0:
                BOOST_TEST(connack_sp_true == wp);
                break;
            case 1:
                BOOST_TEST(publish1dup == wp);
                break;
            case 2:
                BOOST_TEST(publish2dup == wp);
                p.set_value();
                break;
            default:
                BOOST_TEST(false);
                break;
            }
        }
    );
    {
        auto [ec] = ep2.async_send(connack_sp_true, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }
    f.get();
    BOOST_TEST(index == 3);
    {
        auto [ec, pid] = ep2.async_acquire_unique_packet_id(as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
        BOOST_TEST(pid == 3); // 1 and 2 are used by publish(dup)
    }

    // recv pubrec2
    {
        auto [ec, pv] = ep2.async_recv(as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
        BOOST_TEST(pubrec2 == pv);
    }

    // send pubrel2
    ep2.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(pubrel2 == wp);
        }
    );
    {
        auto [ec] = ep2.async_send(pubrel2, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }

    // send pubrel3
    ep2.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(pubrel3 == wp);
        }
    );
    {
        auto [ec] = ep2.async_send(pubrel3, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }

    // recv close
    {
        auto [ec, pv] = ep2.async_recv(as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::errc::connection_reset);
    }

    // recv connect_no_clean
    {
        auto [ec, pv] = ep3.async_recv(as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
        BOOST_TEST(connect_no_clean == pv);
    }

    // get_stored and restore next endpoint
    {
        auto [ec, pvs] = ep2.async_get_stored_packets(as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
        ep3.async_restore_packets(am::force_move(pvs), as::as_tuple(as::use_future));
    }

    // send connack_sp_true
    index = 0;
    p = std::promise<void>();
    f = p.get_future();
    ep3.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            switch (index++) {
            case 0:
                BOOST_TEST(connack_sp_true == wp);
                break;
            case 1:
                BOOST_TEST(publish1dup == wp);
                break;
            case 2:
                BOOST_TEST(pubrel2 == wp);
                break;
            case 3:
                BOOST_TEST(pubrel3 == wp);
                p.set_value();
                break;
            default:
                BOOST_TEST(false);
                break;
            }
        }
    );
    {
        auto [ec] = ep3.async_send(connack_sp_true, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }
    f.get();
    BOOST_TEST(index == 4);
    {
        auto [ec, pid] = ep3.async_acquire_unique_packet_id(as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
        BOOST_TEST(pid == 4); // 1, 2 and 3 are used by publish(dup) and pubrel
    }

    // recv close
    {
        auto [ec, pv] = ep3.async_recv(as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::errc::connection_reset);
    }

    // recv connect_clean
    {
        auto [ec, pv] = ep4.async_recv(as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
        BOOST_TEST(connect_clean == pv);
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
                BOOST_TEST(connack_sp_false == wp);
                p.set_value();
                break;
            default:
                BOOST_TEST(false);
                break;
            }
        }
    );
    {
        auto [ec] = ep4.async_send(connack_sp_false, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }
    f.get();
    BOOST_TEST(index == 1);
    ep1.async_close(as::as_tuple(as::use_future)).get();
    ep2.async_close(as::as_tuple(as::use_future)).get();
    ep3.async_close(as::as_tuple(as::use_future)).get();
    ep4.async_close(as::as_tuple(as::use_future)).get();
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

    auto ep = am::endpoint<async_mqtt::role::client, async_mqtt::stub_socket>{
        version,
        // for stub_socket args
        version,
        ioc.get_executor()
    };

    auto connect = am::v5::connect_packet{
        false,   // clean_session
        0x1234, // keep_alive
        "cid1",
        std::nullopt, // will
        "user1",
        "pass1",
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

    ep.next_layer().set_recv_packets(
        {
            // receive packets
            {connack_sp_false},
        }
    );

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

    // recv connack_sp_false
    {
        auto [ec, pv] = ep.async_recv(as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
        BOOST_TEST(connack_sp_false == pv);
    }

    auto publish0 = am::v5::publish_packet(
        0x0, // packet_id
        "topic1",
        "payload0",
        am::qos::at_most_once,
        am::properties{}
    );

    auto [ec1, pid1] = ep.async_acquire_unique_packet_id(as::as_tuple(as::use_future)).get();
    BOOST_TEST(!ec1);
    auto publish1 = am::v5::publish_packet(
        pid1,
        "topic1",
        "payload1",
        am::qos::at_least_once,
        am::properties{}
    );

    auto publish1dup{publish1};
    publish1dup.set_dup(true);

    auto [ec2, pid2] = ep.async_acquire_unique_packet_id(as::as_tuple(as::use_future)).get();
    BOOST_TEST(!ec2);
    auto publish2 = am::v5::publish_packet(
        pid2,
        "topic1",
        "payload2",
        am::qos::exactly_once,
        am::properties{}
    );

    auto publish2dup{publish2};
    publish2dup.set_dup(true);

    auto pubrec2 = am::v5::pubrec_packet(
        pid2
    );

    auto pubrel2 = am::v5::pubrel_packet(
        pid2
    );

    auto [ec3, pid3] = ep.async_acquire_unique_packet_id(as::as_tuple(as::use_future)).get();
    BOOST_TEST(!ec3);
    auto pubrel3 = am::v5::pubrel_packet(
        pid3
    );

    ep.next_layer().set_recv_packets(
        {
            // receive packets
            {am::errc::make_error_code(am::errc::connection_reset)},
            {connack_sp_true},
            {pubrec2},
            {am::errc::make_error_code(am::errc::connection_reset)},
            {connack_sp_true},
            {am::errc::make_error_code(am::errc::connection_reset)},
            {connack_sp_false},
        }
    );

    // send publish0
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(publish0 == wp);
        }
    );
    {
        auto [ec] = ep.async_send(publish0, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }

    // send publish1
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(publish1 == wp);
        }
    );
    {
        auto [ec] = ep.async_send(publish1, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }

    // send publish2
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(publish2 == wp);
        }
    );
    {
        auto [ec] = ep.async_send(publish2, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }

    // recv close
    {
        auto [ec, pv] = ep.async_recv(as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::errc::connection_reset);
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

    // recv connack_sp_true
    std::size_t index = 0;
    std::promise<void> p;
    auto f = p.get_future();
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            switch (index++) {
            case 0:
                BOOST_TEST(publish1dup == wp);
                break;
            case 1:
                BOOST_TEST(publish2dup == wp);
                p.set_value();
                break;
            default:
                BOOST_TEST(false);
                break;
            }
        }
    );
    {
        auto [ec, pv] = ep.async_recv(as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
        BOOST_TEST(connack_sp_true == pv);
    }
    f.get();
    BOOST_TEST(index == 2);

    // recv pubrec2
    {
        auto [ec, pv] = ep.async_recv(as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
        BOOST_TEST(pubrec2 == pv);
    }

    // send pubrel2
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(pubrel2 == wp);
        }
    );
    {
        auto [ec] = ep.async_send(pubrel2, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }

    // send pubrel3
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(pubrel3 == wp);
        }
    );
    {
        auto [ec] = ep.async_send(pubrel3, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }

    // recv close
    {
        auto [ec, pv] = ep.async_recv(as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::errc::connection_reset);
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

    // recv connack_sp_true
    index = 0;
    p = std::promise<void>();
    f = p.get_future();
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            switch (index++) {
            case 0:
                BOOST_TEST(publish1dup == wp);
                break;
            case 1:
                BOOST_TEST(pubrel2 == wp);
                break;
            case 2:
                BOOST_TEST(pubrel3 == wp);
                p.set_value();
                break;
            default:
                BOOST_TEST(false);
                break;
            }
        }
    );
    {
        auto [ec, pv] = ep.async_recv(as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
        BOOST_TEST(connack_sp_true == pv);
    }
    f.get();
    BOOST_TEST(index == 3);

    // recv close
    {
        auto [ec, pv] = ep.async_recv(as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::errc::connection_reset);
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

    // recv connack_sp_false
    index = 0;
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec, pv] = ep.async_recv(as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
        BOOST_TEST(connack_sp_false == pv);
    }
    ep.async_close(as::as_tuple(as::use_future)).get();
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

    auto ep1 = am::endpoint<async_mqtt::role::server, async_mqtt::stub_socket>{
        version,
        // for stub_socket args
        version,
        ioc.get_executor()
    };
    auto ep2 = am::endpoint<async_mqtt::role::server, async_mqtt::stub_socket>{
        version,
        // for stub_socket args
        version,
        ioc.get_executor()
    };
    auto ep3 = am::endpoint<async_mqtt::role::server, async_mqtt::stub_socket>{
        version,
        // for stub_socket args
        version,
        ioc.get_executor()
    };
    auto ep4 = am::endpoint<async_mqtt::role::server, async_mqtt::stub_socket>{
        version,
        // for stub_socket args
        version,
        ioc.get_executor()
    };

    auto connect_no_clean = am::v5::connect_packet{
        false,   // clean_session
        0x1234, // keep_alive
        "cid1",
        std::nullopt, // will
        "user1",
        "pass1",
        am::properties{
            am::property::session_expiry_interval{am::session_never_expire}
        }
    };

    auto connect_clean = am::v5::connect_packet{
        true,   // clean_session
        0x1234, // keep_alive
        "cid1",
        std::nullopt, // will
        "user1",
        "pass1",
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

    ep1.next_layer().set_recv_packets(
        {
            // receive packets
            {connect_no_clean},
        }
    );

    // recv connect_no_clean
    {
        auto [ec, pv] = ep1.async_recv(as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
        BOOST_TEST(connect_no_clean == pv);
    }

    // send connack_sp_false
    ep1.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(connack_sp_false == wp);
        }
    );
    {
        auto [ec] = ep1.async_send(connack_sp_false, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }

    auto publish0 = am::v5::publish_packet(
        0x0, // packet_id
        "topic1",
        "payload0",
        am::qos::at_most_once,
        am::properties{}
    );

    auto [ec1, pid1] = ep1.async_acquire_unique_packet_id(as::as_tuple(as::use_future)).get();
    BOOST_TEST(!ec1);
    auto publish1 = am::v5::publish_packet(
        pid1,
        "topic1",
        "payload1",
        am::qos::at_least_once,
        am::properties{}
    );

    auto publish1dup{publish1};
    publish1dup.set_dup(true);

    auto [ec2, pid2] = ep1.async_acquire_unique_packet_id(as::as_tuple(as::use_future)).get();
    BOOST_TEST(!ec2);
    auto publish2 = am::v5::publish_packet(
        pid2,
        "topic1",
        "payload2",
        am::qos::exactly_once,
        am::properties{}
    );

    auto publish2dup{publish2};
    publish2dup.set_dup(true);

    auto pubrec2 = am::v5::pubrec_packet(
        pid2
    );

    auto pubrel2 = am::v5::pubrel_packet(
        pid2
    );

    auto [ec3, pid3] = ep1.async_acquire_unique_packet_id(as::as_tuple(as::use_future)).get();
    BOOST_TEST(!ec3);
    auto pubrel3 = am::v5::pubrel_packet(
        pid3
    );

    ep1.next_layer().set_recv_packets(
        {
            // receive packets
            {am::errc::make_error_code(am::errc::connection_reset)},
        }
    );
    ep2.next_layer().set_recv_packets(
        {
            // receive packets
            {connect_no_clean},
            {pubrec2},
            {am::errc::make_error_code(am::errc::connection_reset)},
        }
    );
    ep3.next_layer().set_recv_packets(
        {
            // receive packets
            {connect_no_clean},
            {am::errc::make_error_code(am::errc::connection_reset)},
        }
    );
    ep4.next_layer().set_recv_packets(
        {
            // receive packets
            {connect_clean},
        }
    );

    // send publish0
    ep1.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(publish0 == wp);
        }
    );
    {
        auto [ec] = ep1.async_send(publish0, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }

    // send publish1
    ep1.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(publish1 == wp);
        }
    );
    {
        auto [ec] = ep1.async_send(publish1, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }

    // send publish2
    ep1.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(publish2 == wp);
        }
    );
    {
        auto [ec] = ep1.async_send(publish2, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }

    // recv close
    {
        auto [ec, pv] = ep1.async_recv(as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::errc::connection_reset);
    }

    // recv connect_no_clean
    {
        auto [ec, pv] = ep2.async_recv(as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
        BOOST_TEST(connect_no_clean == pv);
    }

    // get_stored and restore next endpoint
    {
        auto [ec, pvs] = ep1.async_get_stored_packets(as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
        ep2.async_restore_packets(am::force_move(pvs), as::as_tuple(as::use_future));
    }

    // send connack_sp_true
    std::size_t index = 0;
    std::promise<void> p;
    auto f = p.get_future();
    ep2.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            switch (index++) {
            case 0:
                BOOST_TEST(connack_sp_true == wp);
                break;
            case 1:
                BOOST_TEST(publish1dup == wp);
                break;
            case 2:
                BOOST_TEST(publish2dup == wp);
                p.set_value();
                break;
            default:
                BOOST_TEST(false);
                break;
            }
        }
    );
    {
        auto [ec] = ep2.async_send(connack_sp_true, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }
    f.get();
    BOOST_TEST(index == 3);

    {
        auto [ec, pid] = ep2.async_acquire_unique_packet_id(as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
        BOOST_TEST(pid == 3); // 1 and 2 are used by publish(dup)
    }

    // recv pubrec2
    {
        auto [ec, pv] = ep2.async_recv(as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
        BOOST_TEST(pubrec2 == pv);
    }

    // send pubrel2
    ep2.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(pubrel2 == wp);
        }
    );
    {
        auto [ec] = ep2.async_send(pubrel2, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }

    // send pubrel3
    ep2.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(pubrel3 == wp);
        }
    );
    {
        auto [ec] = ep2.async_send(pubrel3, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }

    // recv close
    {
        auto [ec, pv] = ep2.async_recv(as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::errc::connection_reset);
    }

    // recv connect_no_clean
    {
        auto [ec, pv] = ep3.async_recv(as::as_tuple(as::use_future)).get();
        BOOST_TEST(connect_no_clean == pv);
    }

    // get_stored and restore next endpoint
    {
        auto [ec, pvs] = ep2.async_get_stored_packets(as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
        ep3.async_restore_packets(am::force_move(pvs), as::as_tuple(as::use_future));
    }

    // send connack_sp_true
    index = 0;
    p = std::promise<void>();
    f = p.get_future();
    ep3.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            switch (index++) {
            case 0:
                BOOST_TEST(connack_sp_true == wp);
                break;
            case 1:
                BOOST_TEST(publish1dup == wp);
                break;
            case 2:
                BOOST_TEST(pubrel2 == wp);
                break;
            case 3:
                BOOST_TEST(pubrel3 == wp);
                p.set_value();
                break;
            default:
                BOOST_TEST(false);
                break;
            }
        }
    );
    {
        auto [ec] = ep3.async_send(connack_sp_true, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }
    f.get();
    BOOST_TEST(index == 4);
    {
        auto [ec, pid] = ep3.async_acquire_unique_packet_id(as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
        BOOST_TEST(pid == 4); // 1, 2 and 3 are used by publish(dup) and pubrel
    }

    // recv close
    {
        auto [ec, pv] = ep3.async_recv(as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::errc::connection_reset);
    }

    // recv connect_clean
    {
        auto [ec, pv] = ep4.async_recv(as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
        BOOST_TEST(connect_clean == pv);
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
                BOOST_TEST(connack_sp_false == wp);
                p.set_value();
                break;
            default:
                BOOST_TEST(false);
                break;
            }
        }
    );
    {
        auto [ec] = ep4.async_send(connack_sp_false, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }
    f.get();
    BOOST_TEST(index == 1);
    ep1.async_close(as::as_tuple(as::use_future)).get();
    ep2.async_close(as::as_tuple(as::use_future)).get();
    ep3.async_close(as::as_tuple(as::use_future)).get();
    ep4.async_close(as::as_tuple(as::use_future)).get();
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

    auto ep = am::endpoint<async_mqtt::role::client, async_mqtt::stub_socket>{
        version,
        // for stub_socket args
        version,
        ioc.get_executor()
    };

    auto connect = am::v5::connect_packet{
        false,   // clean_session
        0x1234, // keep_alive
        "cid1",
        std::nullopt, // will
        "user1",
        "pass1",
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

    ep.next_layer().set_recv_packets(
        {
            // receive packets
            {connack_sp_false},
        }
    );

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

    // recv connack_sp_false
    {
        auto [ec, pv] = ep.async_recv(as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
        BOOST_TEST(connack_sp_false == pv);
    }

    auto publish0 = am::v5::publish_packet(
        0x0, // packet_id
        "topic1",
        "payload0",
        am::qos::at_most_once,
        am::properties{}
    );

    auto [ec1, pid1] = ep.async_acquire_unique_packet_id(as::as_tuple(as::use_future)).get();
    BOOST_TEST(!ec1);
    auto publish1 = am::v5::publish_packet(
        pid1,
        "topic1",
        "payload1",
        am::qos::at_least_once,
        am::properties{
            am::property::topic_alias{1}
        }
    );

    auto publish1no_ta = am::v5::publish_packet(
        pid1,
        "topic1",
        "payload1",
        am::qos::at_least_once,
        am::properties{}
    );

    auto publish1no_ta_dup{publish1no_ta};
    publish1no_ta_dup.set_dup(true);

    auto [ec2, pid2] = ep.async_acquire_unique_packet_id(as::as_tuple(as::use_future)).get();
    BOOST_TEST(!ec2);
    auto publish2 = am::v5::publish_packet(
        pid2,
        "",
        "payload2",
        am::qos::exactly_once,
        am::properties{
            am::property::topic_alias{1}
        }
    );

    auto publish2no_ta = am::v5::publish_packet(
        pid2,
        "topic1",
        "payload2",
        am::qos::exactly_once,
        am::properties{}
    );

    auto publish2no_ta_dup{publish2no_ta};
    publish2no_ta_dup.set_dup(true);

    auto pubrec2 = am::v5::pubrec_packet(
        pid2
    );

    auto pubrel2 = am::v5::pubrel_packet(
        pid2
    );

    auto [ec3, pid3] = ep.async_acquire_unique_packet_id(as::as_tuple(as::use_future)).get();
    BOOST_TEST(!ec3);
    auto pubrel3 = am::v5::pubrel_packet(
        pid3
    );

    ep.next_layer().set_recv_packets(
        {
            // receive packets
            {am::errc::make_error_code(am::errc::connection_reset)},
            {connack_sp_true},
            {pubrec2},
            {am::errc::make_error_code(am::errc::connection_reset)},
            {connack_sp_true},
            {am::errc::make_error_code(am::errc::connection_reset)},
            {connack_sp_false},
        }
    );

    // send publish0
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(publish0 == wp);
        }
    );
    {
        auto [ec] = ep.async_send(publish0, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }

    // send publish1
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(publish1 == wp);
        }
    );
    {
        auto [ec] = ep.async_send(publish1, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }

    // send publish2
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(publish2 == wp);
        }
    );
    {
        auto [ec] = ep.async_send(publish2, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }

    // recv close
    {
        auto [ec, pv] = ep.async_recv(as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::errc::connection_reset);
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

    // recv connack_sp_true
    std::size_t index = 0;
    std::promise<void> p;
    auto f = p.get_future();
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            switch (index++) {
            case 0:
                BOOST_TEST(publish1no_ta_dup == wp);
                break;
            case 1:
                BOOST_TEST(publish2no_ta_dup == wp);
                p.set_value();
                break;
            default:
                BOOST_TEST(false);
                break;
            }
        }
    );
    {
        auto [ec, pv] = ep.async_recv(as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
        BOOST_TEST(connack_sp_true == pv);
    }
    f.get();
    BOOST_TEST(index == 2);

    // recv pubrec2
    {
        auto [ec, pv] = ep.async_recv(as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
        BOOST_TEST(pubrec2 == pv);
    }

    // send pubrel2
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(pubrel2 == wp);
        }
    );
    {
        auto [ec] = ep.async_send(pubrel2, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }

    // send pubrel3
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(pubrel3 == wp);
        }
    );
    {
        auto [ec] = ep.async_send(pubrel3, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }

    // recv close
    {
        auto [ec, pv] = ep.async_recv(as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::errc::connection_reset);
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

    // recv connack_sp_true
    index = 0;
    p = std::promise<void>();
    f = p.get_future();
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            switch (index++) {
            case 0:
                BOOST_TEST(publish1no_ta_dup == wp);
                break;
            case 1:
                BOOST_TEST(pubrel2 == wp);
                break;
            case 2:
                BOOST_TEST(pubrel3 == wp);
                p.set_value();
                break;
            default:
                BOOST_TEST(false);
                break;
            }
        }
    );
    {
        auto [ec, pv] = ep.async_recv(as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
        BOOST_TEST(connack_sp_true == pv);
    }
    f.get();
    BOOST_TEST(index == 3);

    // recv close
    {
        auto [ec, pv] = ep.async_recv(as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::errc::connection_reset);
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

    // recv connack_sp_false
    index = 0;
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec, pv] = ep.async_recv(as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
        BOOST_TEST(connack_sp_false == pv);
    }
    ep.async_close(as::as_tuple(as::use_future)).get();
    guard.reset();
    th.join();
}

BOOST_AUTO_TEST_CASE(restore_packets_error) {
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

    auto [ec1, pid1] = ep.async_acquire_unique_packet_id(as::as_tuple(as::use_future)).get();
    BOOST_TEST(!ec1);
    auto publish1 = am::v3_1_1::publish_packet(
        pid1,
        "topic1",
        "payload1",
        am::qos::at_least_once
    );
    as::dispatch(
        as::bind_executor(
            ep.get_executor(),
            [&] {
                ep.restore_packets({am::store_packet_variant{publish1}}); // pid is already used
                auto stored = ep.get_stored_packets();
                BOOST_TEST(stored.empty());
            }
        )
    );
    guard.reset();
    th.join();
}

BOOST_AUTO_TEST_SUITE_END()
