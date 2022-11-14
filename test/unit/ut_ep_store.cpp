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

BOOST_AUTO_TEST_SUITE(ut_ep_store)

namespace am = async_mqtt;
namespace as = boost::asio;

BOOST_AUTO_TEST_CASE(v3_1_1_client) {
    auto version = am::protocol_version::v3_1_1;
    as::io_context ioc;
    auto guard = as::make_work_guard(ioc.get_executor());
    std::thread th {
        [&] {
            ioc.run();
        }
    };

    auto connect = am::v3_1_1::connect_packet{
        false,   // clean_session
        0x1234, // keep_alive
        am::allocate_buffer("cid1"),
        am::nullopt, // will
        am::allocate_buffer("user1"),
        am::allocate_buffer("pass1")
    };

    auto connack1 = am::v3_1_1::connack_packet{
        false,   // session_present
        am::connect_return_code::accepted
    };

    auto connack2 = am::v3_1_1::connack_packet{
        true,   // session_present
        am::connect_return_code::accepted
    };

    auto close = am::make_error(am::errc::network_reset, "pseudo close");

    auto publish0 = am::v3_1_1::publish_packet(
        0x0, // packet_id
        am::allocate_buffer("topic1"),
        am::allocate_buffer("payload0"),
        am::qos::at_most_once
    );

    auto publish1 = am::v3_1_1::publish_packet(
        0x1, // packet_id
        am::allocate_buffer("topic1"),
        am::allocate_buffer("payload1"),
        am::qos::at_least_once
    );

    auto publish2 = am::v3_1_1::publish_packet(
        0x2, // packet_id
        am::allocate_buffer("topic1"),
        am::allocate_buffer("payload2"),
        am::qos::exactly_once
    );

    auto pubrec2 = am::v3_1_1::pubrec_packet(
        0x2 // packet_id
    );

    auto pubrel2 = am::v3_1_1::pubrel_packet(
        0x2 // packet_id
    );

    auto pubrel5 = am::v3_1_1::pubrel_packet(
        0x5 // packet_id
    );

    am::endpoint<async_mqtt::role::client, async_mqtt::stub_socket> ep{
        version,
        // for stub_socket args
        version,
        ioc,
        std::deque<am::packet_variant> {
            // receive packets
            connack1,
            close,
            connack2,
            pubrec2,
            close,
            connack2,
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

    // recv connack1
    {
        auto pv = ep.recv(as::use_future).get();
        BOOST_TEST(am::packet_compare(connack1, pv));
    }

    // send publish0
    ep.stream().next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(publish0, wp));
        }
    );
    {
        auto ec = ep.send(publish0, as::use_future).get();
        BOOST_TEST(!ec);
    }

    // send publish1
    ep.stream().next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(publish1, wp));
        }
    );
    {
        auto ec = ep.send(publish1, as::use_future).get();
        BOOST_TEST(!ec);
    }

    // send publish2
    ep.stream().next_layer().set_write_packet_checker(
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
    ep.stream().next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(connect, wp));
        }
    );
    {
        auto ec = ep.send(connect, as::use_future).get();
        BOOST_TEST(!ec);
    }

    // recv connack2
    std::size_t index = 0;
    ep.stream().next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            switch (index++) {
            case 0:
                BOOST_TEST(am::packet_compare(publish1, wp));
                break;
            case 1:
                BOOST_TEST(am::packet_compare(publish2, wp));
                break;
            default:
                BOOST_TEST(false);
                break;
            }
        }
    );
    {
        auto pv = ep.recv(as::use_future).get();
        BOOST_TEST(am::packet_compare(connack2, pv));
    }

    // recv pubrec2
    {
        auto pv = ep.recv(as::use_future).get();
        BOOST_TEST(am::packet_compare(pubrec2, pv));
    }

    // send pubrel2
    ep.stream().next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(pubrel2, wp));
        }
    );
    {
        auto ec = ep.send(pubrel2, as::use_future).get();
        BOOST_TEST(!ec);
    }

    // send pubrel5
    ep.stream().next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(pubrel5, wp));
        }
    );
    {
        auto ec = ep.send(pubrel5, as::use_future).get();
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

    // recv connack2
    index = 0;
    ep.stream().next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            switch (index++) {
            case 0:
                BOOST_TEST(am::packet_compare(publish1, wp));
                break;
            case 1:
                BOOST_TEST(am::packet_compare(pubrel2, wp));
                break;
            case 2:
                BOOST_TEST(am::packet_compare(pubrel5, wp));
                break;
            default:
                BOOST_TEST(false);
                break;
            }
        }
    );
    {
        auto pv = ep.recv(as::use_future).get();
        BOOST_TEST(am::packet_compare(connack2, pv));
    }

    guard.reset();
    th.join();
}


BOOST_AUTO_TEST_SUITE_END()
