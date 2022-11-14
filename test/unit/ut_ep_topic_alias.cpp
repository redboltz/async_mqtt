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

BOOST_AUTO_TEST_SUITE(ut_ep_topic_alias)

namespace am = async_mqtt;
namespace as = boost::asio;

BOOST_AUTO_TEST_CASE(send) {
    auto version = am::protocol_version::v5;
    as::io_context ioc;
    auto guard = as::make_work_guard(ioc.get_executor());
    std::thread th {
        [&] {
            ioc.run();
        }
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
        true,   // session_present
        am::connect_reason_code::success,
        am::properties{
            am::property::topic_alias_maximum{2}
        }
    };

    auto publish_reg_t1 = am::v5::publish_packet(
        0x1234, // packet_id
        am::allocate_buffer("topic1"),
        am::allocate_buffer("payload1"),
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{
            am::property::topic_alias{1}
        }
    );

    auto publish_use_ta1 = am::v5::publish_packet(
        0x1234, // packet_id
        am::buffer{},
        am::buffer("payload1"),
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{
            am::property::topic_alias{1}
        }
    );

    auto publish_reg_t2 = am::v5::publish_packet(
        0x1234, // packet_id
        am::allocate_buffer("topic2"),
        am::allocate_buffer("payload1"),
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{
            am::property::topic_alias{2}
        }
    );

    auto publish_use_ta2 = am::v5::publish_packet(
        0x1234, // packet_id
        am::buffer{},
        am::buffer("payload1"),
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{
            am::property::topic_alias{2}
        }
    );

    auto publish_reg_t3 = am::v5::publish_packet(
        0x1234, // packet_id
        am::allocate_buffer("topic3"),
        am::allocate_buffer("payload1"),
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{
            am::property::topic_alias{3} // over
        }
    );

    auto publish_use_t3 = am::v5::publish_packet(
        0x1234, // packet_id
        am::buffer{},
        am::buffer("payload1"),
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{
            am::property::topic_alias{3} // over
        }
    );

    auto publish_upd_t3 = am::v5::publish_packet(
        0x1234, // packet_id
        am::allocate_buffer("topic3"),
        am::allocate_buffer("payload1"),
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{
            am::property::topic_alias{1} // update
        }
    );

    am::endpoint<async_mqtt::role::client, async_mqtt::stub_socket> ep{
        version,
        // for stub_socket args
        version,
        ioc,
        std::deque<am::packet_variant> {
            // receive packets
            connack,
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

    // recv connack
    {
        auto pv = ep.recv(as::use_future).get();
        BOOST_TEST(am::packet_compare(connack, pv));
    }

    // send publish_reg_t1
    ep.stream().next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(publish_reg_t1, wp));
        }
    );
    {
        auto ec = ep.send(publish_reg_t1, as::use_future).get();
        BOOST_TEST(!ec);
    }

    // send publish_use_ta1
    ep.stream().next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(publish_use_ta1, wp));
        }
    );
    {
        auto ec = ep.send(publish_use_ta1, as::use_future).get();
        BOOST_TEST(!ec);
    }

    // send publish_reg_t2
    ep.stream().next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(publish_reg_t2, wp));
        }
    );
    {
        auto ec = ep.send(publish_reg_t2, as::use_future).get();
        BOOST_TEST(!ec);
    }

    // send publish_use_ta2
    ep.stream().next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(publish_use_ta2, wp));
        }
    );
    {
        auto ec = ep.send(publish_use_ta2, as::use_future).get();
        BOOST_TEST(!ec);
    }

    // send publish_reg_t3
    ep.stream().next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(publish_reg_t3, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::bad_message);
    }

    // send publish_use_t3
    ep.stream().next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(publish_use_t3, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::bad_message);
    }

    // send publish_upd_t3
    ep.stream().next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(publish_upd_t3, wp));
        }
    );
    {
        auto ec = ep.send(publish_upd_t3, as::use_future).get();
        BOOST_TEST(!ec);
    }

    guard.reset();
    th.join();
}

BOOST_AUTO_TEST_CASE(send_auto_map) {
    auto version = am::protocol_version::v5;
    as::io_context ioc;
    auto guard = as::make_work_guard(ioc.get_executor());
    std::thread th {
        [&] {
            ioc.run();
        }
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
        true,   // session_present
        am::connect_reason_code::success,
        am::properties{
            am::property::topic_alias_maximum{2}
        }
    };

    auto publish_t1 = am::v5::publish_packet(
        0x1234, // packet_id
        am::allocate_buffer("topic1"),
        am::allocate_buffer("payload1"),
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{}
    );

    auto publish_mapped_ta1 = am::v5::publish_packet(
        0x1234, // packet_id
        am::allocate_buffer("topic1"),
        am::buffer("payload1"),
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{
            am::property::topic_alias{1}
        }
    );

    auto publish_use_ta1 = am::v5::publish_packet(
        0x1234, // packet_id
        am::buffer{},
        am::buffer("payload1"),
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{
            am::property::topic_alias{1}
        }
    );

    auto publish_t2 = am::v5::publish_packet(
        0x1234, // packet_id
        am::allocate_buffer("topic2"),
        am::allocate_buffer("payload1"),
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{}
    );

    auto publish_mapped_ta2 = am::v5::publish_packet(
        0x1234, // packet_id
        am::allocate_buffer("topic2"),
        am::buffer("payload1"),
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{
            am::property::topic_alias{2}
        }
    );

    auto publish_use_ta2 = am::v5::publish_packet(
        0x1234, // packet_id
        am::buffer{},
        am::buffer("payload1"),
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{
            am::property::topic_alias{2}
        }
    );

    auto publish_t3 = am::v5::publish_packet(
        0x1234, // packet_id
        am::allocate_buffer("topic3"),
        am::allocate_buffer("payload1"),
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{}
    );

    auto publish_mapped_ta1_2 = am::v5::publish_packet(
        0x1234, // packet_id
        am::allocate_buffer("topic3"),
        am::buffer("payload1"),
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{
            am::property::topic_alias{1}
        }
    );

    auto publish_use_ta1_2 = am::v5::publish_packet(
        0x1234, // packet_id
        am::buffer{},
        am::buffer("payload1"),
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{
            am::property::topic_alias{1}
        }
    );

    am::endpoint<async_mqtt::role::client, async_mqtt::stub_socket> ep{
        version,
        // for stub_socket args
        version,
        ioc,
        std::deque<am::packet_variant> {
            // receive packets
            connack,
        }
    };
    ep.set_auto_map_topic_alias_send(true);

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

    // send publish_t1
    ep.stream().next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(publish_mapped_ta1, wp));
        }
    );
    {
        auto ec = ep.send(publish_t1, as::use_future).get();
        BOOST_TEST(!ec);
    }

    // send publish_t1
    ep.stream().next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(publish_use_ta1, wp));
        }
    );
    {
        auto ec = ep.send(publish_t1, as::use_future).get();
        BOOST_TEST(!ec);
    }

    // send publish_t2
    ep.stream().next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(publish_mapped_ta2, wp));
        }
    );
    {
        auto ec = ep.send(publish_t2, as::use_future).get();
        BOOST_TEST(!ec);
    }

    // send publish_t2
    ep.stream().next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(publish_use_ta2, wp));
        }
    );
    {
        auto ec = ep.send(publish_t2, as::use_future).get();
        BOOST_TEST(!ec);
    }

    // send publish_t3
    ep.stream().next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(publish_mapped_ta1_2, wp));
        }
    );
    {
        auto ec = ep.send(publish_t3, as::use_future).get();
        BOOST_TEST(!ec);
    }

    // send publish_t3
    ep.stream().next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(publish_use_ta1_2, wp));
        }
    );
    {
        auto ec = ep.send(publish_t3, as::use_future).get();
        BOOST_TEST(!ec);
    }

    guard.reset();
    th.join();
}

BOOST_AUTO_TEST_CASE(send_auto_replace) {
    auto version = am::protocol_version::v5;
    as::io_context ioc;
    auto guard = as::make_work_guard(ioc.get_executor());
    std::thread th {
        [&] {
            ioc.run();
        }
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
        true,   // session_present
        am::connect_reason_code::success,
        am::properties{
            am::property::topic_alias_maximum{2}
        }
    };

    auto publish_t1 = am::v5::publish_packet(
        0x1234, // packet_id
        am::allocate_buffer("topic1"),
        am::allocate_buffer("payload1"),
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{}
    );

    auto publish_map_ta1 = am::v5::publish_packet(
        0x1234, // packet_id
        am::allocate_buffer("topic1"),
        am::buffer("payload1"),
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{
            am::property::topic_alias{1}
        }
    );

    auto publish_use_ta1 = am::v5::publish_packet(
        0x1234, // packet_id
        am::buffer{},
        am::buffer("payload1"),
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{
            am::property::topic_alias{1}
        }
    );

    am::endpoint<async_mqtt::role::client, async_mqtt::stub_socket> ep{
        version,
        // for stub_socket args
        version,
        ioc,
        std::deque<am::packet_variant> {
            // receive packets
            connack,
        }
    };
    ep.set_auto_replace_topic_alias_send(true);

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

    // send publish_t1
    ep.stream().next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(publish_t1, wp));
        }
    );
    {
        auto ec = ep.send(publish_t1, as::use_future).get();
        BOOST_TEST(!ec);
    }

    // send publish_use_ta1
    ep.stream().next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(publish_use_ta1, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::bad_message);
    }

    // send publish_map_ta1
    ep.stream().next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(publish_map_ta1, wp));
        }
    );
    {
        auto ec = ep.send(publish_map_ta1, as::use_future).get();
        BOOST_TEST(!ec);
    }

    // send publish_t1
    ep.stream().next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            // auto use
            BOOST_TEST(am::packet_compare(publish_use_ta1, wp));
        }
    );
    {
        auto ec = ep.send(publish_t1, as::use_future).get();
        BOOST_TEST(!ec);
    }

    guard.reset();
    th.join();
}

BOOST_AUTO_TEST_SUITE_END()
