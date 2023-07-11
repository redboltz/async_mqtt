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
using namespace am::literals;

inline am::optional<am::topic_alias_t> get_topic_alias(am::properties const& props) {
    am::optional<am::topic_alias_t> ta_opt;
    for (auto const& prop : props) {
        prop.visit(
            am::overload {
                [&](am::property::topic_alias const& p) {
                    ta_opt.emplace(p.val());
                },
                [](auto const&) {
                }
            }
        );
        if (ta_opt) return ta_opt;
    }
    return ta_opt;
}

BOOST_AUTO_TEST_CASE(send_client) {
    auto version = am::protocol_version::v5;
    as::io_context ioc;
    auto guard = as::make_work_guard(ioc.get_executor());
    std::thread th {
        [&] {
            ioc.run();
        }
    };

    using ep_t = am::endpoint<async_mqtt::role::client, async_mqtt::stub_socket>;
    ep_t ep{
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
        true,   // session_present
        am::connect_reason_code::success,
        am::properties{
            am::property::topic_alias_maximum{2}
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
    auto publish_reg_t1 = am::v5::publish_packet(
        *pid_opt1,
        am::allocate_buffer("topic1"),
        am::allocate_buffer("payload1"),
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{
            am::property::topic_alias{1}
        }
    );

    auto pid_opt2 = ep.acquire_unique_packet_id(as::use_future).get();
    BOOST_TEST(pid_opt2.has_value());
    auto publish_use_ta1 = am::v5::publish_packet(
        *pid_opt2,
        am::buffer{},
        am::buffer("payload1"),
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{
            am::property::topic_alias{1}
        }
    );

    auto pid_opt3 = ep.acquire_unique_packet_id(as::use_future).get();
    BOOST_TEST(pid_opt3.has_value());
    auto publish_reg_t2 = am::v5::publish_packet(
        *pid_opt3,
        am::allocate_buffer("topic2"),
        am::allocate_buffer("payload1"),
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{
            am::property::topic_alias{2}
        }
    );

    auto pid_opt4 = ep.acquire_unique_packet_id(as::use_future).get();
    BOOST_TEST(pid_opt4.has_value());
    auto publish_use_ta2 = am::v5::publish_packet(
        *pid_opt4,
        am::buffer{},
        am::buffer("payload1"),
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{
            am::property::topic_alias{2}
        }
    );

    auto pid_opt5 = ep.acquire_unique_packet_id(as::use_future).get();
    BOOST_TEST(pid_opt5.has_value());
    auto publish_reg_t3 = am::v5::publish_packet(
        *pid_opt5,
        am::allocate_buffer("topic3"),
        am::allocate_buffer("payload1"),
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{
            am::property::topic_alias{3} // over
        }
    );

    auto pid_opt6 = ep.acquire_unique_packet_id(as::use_future).get();
    BOOST_TEST(pid_opt6.has_value());
    auto publish_use_ta3 = am::v5::publish_packet(
        *pid_opt6,
        am::buffer{},
        am::buffer("payload1"),
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{
            am::property::topic_alias{3} // over
        }
    );

    auto pid_opt7 = ep.acquire_unique_packet_id(as::use_future).get();
    BOOST_TEST(pid_opt7.has_value());
    auto publish_upd_t3 = am::v5::publish_packet(
        *pid_opt7,
        am::allocate_buffer("topic3"),
        am::allocate_buffer("payload1"),
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{
            am::property::topic_alias{1} // update
        }
    );

    // send publish_reg_t1
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(publish_reg_t1, wp));
        }
    );
    {
        auto ec = ep.send(publish_reg_t1, as::use_future).get();
        BOOST_TEST(!ec);
    }
    {   // check regulate
        auto p = publish_reg_t1;
        auto rp = ep.regulate_for_store(p, as::use_future).get();
        BOOST_TEST(rp.topic() == "topic1"_mb);
        BOOST_TEST(!get_topic_alias(rp.props()));

        // idempotence
        auto p2 = p;
        auto rp2 = ep.regulate_for_store(p2, as::use_future).get();
        BOOST_TEST(rp2.topic() == "topic1"_mb);
        BOOST_TEST(!get_topic_alias(rp2.props()));
    }

    // send publish_use_ta1
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(publish_use_ta1, wp));
        }
    );
    {
        auto ec = ep.send(publish_use_ta1, as::use_future).get();
        BOOST_TEST(!ec);
    }
    {   // check regulate
        auto p = publish_use_ta1;
        auto rp = ep.regulate_for_store(p, as::use_future).get();
        BOOST_TEST(rp.topic() == "topic1"_mb);
        BOOST_TEST(!get_topic_alias(rp.props()));
    }

    // send publish_reg_t2
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(publish_reg_t2, wp));
        }
    );
    {
        auto ec = ep.send(publish_reg_t2, as::use_future).get();
        BOOST_TEST(!ec);
    }

    // send publish_use_ta2
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(publish_use_ta2, wp));
        }
    );
    {
        auto ec = ep.send(publish_use_ta2, as::use_future).get();
        BOOST_TEST(!ec);
    }

    // send publish_reg_t3
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(publish_reg_t3, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::bad_message);
    }

    // send publish_use_ta3
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(publish_use_ta3, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::bad_message);
    }

    // send publish_upd_t3
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(publish_upd_t3, wp));
        }
    );
    {
        auto ec = ep.send(publish_upd_t3, as::use_future).get();
        BOOST_TEST(!ec);
    }
    ep.close(as::use_future).get();
    guard.reset();
    th.join();
}

BOOST_AUTO_TEST_CASE(send_server) {
    auto version = am::protocol_version::v5;
    as::io_context ioc;
    auto guard = as::make_work_guard(ioc.get_executor());
    std::thread th {
        [&] {
            ioc.run();
        }
    };

    using ep_t = am::endpoint<async_mqtt::role::server, async_mqtt::stub_socket>;
    ep_t ep{
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
            am::property::topic_alias_maximum{2}
        }
    };

    auto connack = am::v5::connack_packet{
        true,   // session_present
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
    auto publish_reg_t1 = am::v5::publish_packet(
        *pid_opt1,
        am::allocate_buffer("topic1"),
        am::allocate_buffer("payload1"),
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{
            am::property::topic_alias{1}
        }
    );

    auto pid_opt2 = ep.acquire_unique_packet_id(as::use_future).get();
    BOOST_TEST(pid_opt2.has_value());
    auto publish_use_ta1 = am::v5::publish_packet(
        *pid_opt2,
        am::buffer{},
        am::buffer("payload1"),
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{
            am::property::topic_alias{1}
        }
    );

    auto pid_opt3 = ep.acquire_unique_packet_id(as::use_future).get();
    BOOST_TEST(pid_opt3.has_value());
    auto publish_reg_t2 = am::v5::publish_packet(
        *pid_opt3,
        am::allocate_buffer("topic2"),
        am::allocate_buffer("payload1"),
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{
            am::property::topic_alias{2}
        }
    );

    auto pid_opt4 = ep.acquire_unique_packet_id(as::use_future).get();
    BOOST_TEST(pid_opt4.has_value());
    auto publish_use_ta2 = am::v5::publish_packet(
        *pid_opt4,
        am::buffer{},
        am::buffer("payload1"),
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{
            am::property::topic_alias{2}
        }
    );

    auto pid_opt5 = ep.acquire_unique_packet_id(as::use_future).get();
    BOOST_TEST(pid_opt5.has_value());
    auto publish_reg_t3 = am::v5::publish_packet(
        *pid_opt5,
        am::allocate_buffer("topic3"),
        am::allocate_buffer("payload1"),
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{
            am::property::topic_alias{3} // over
        }
    );

    auto pid_opt6 = ep.acquire_unique_packet_id(as::use_future).get();
    BOOST_TEST(pid_opt6.has_value());
    auto publish_use_ta3 = am::v5::publish_packet(
        *pid_opt6,
        am::buffer{},
        am::buffer("payload1"),
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{
            am::property::topic_alias{3} // over
        }
    );

    auto pid_opt7 = ep.acquire_unique_packet_id(as::use_future).get();
    BOOST_TEST(pid_opt7.has_value());
    auto publish_upd_t3 = am::v5::publish_packet(
        *pid_opt7,
        am::allocate_buffer("topic3"),
        am::allocate_buffer("payload1"),
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{
            am::property::topic_alias{1} // update
        }
    );

    // send publish_reg_t1
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(publish_reg_t1, wp));
        }
    );
    {
        auto ec = ep.send(publish_reg_t1, as::use_future).get();
        BOOST_TEST(!ec);
    }

    // send publish_use_ta1
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(publish_use_ta1, wp));
        }
    );
    {
        auto ec = ep.send(publish_use_ta1, as::use_future).get();
        BOOST_TEST(!ec);
    }

    // send publish_reg_t2
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(publish_reg_t2, wp));
        }
    );
    {
        auto ec = ep.send(publish_reg_t2, as::use_future).get();
        BOOST_TEST(!ec);
    }

    // send publish_use_ta2
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(publish_use_ta2, wp));
        }
    );
    {
        auto ec = ep.send(publish_use_ta2, as::use_future).get();
        BOOST_TEST(!ec);
    }

    // send publish_reg_t3
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(publish_reg_t3, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::bad_message);
    }

    // send publish_use_ta3
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(publish_use_ta3, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::bad_message);
    }

    // send publish_upd_t3
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(publish_upd_t3, wp));
        }
    );
    {
        auto ec = ep.send(publish_upd_t3, as::use_future).get();
        BOOST_TEST(!ec);
    }
    ep.close(as::use_future).get();
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

    using ep_t = am::endpoint<async_mqtt::role::client, async_mqtt::stub_socket>;
    ep_t ep{
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
        true,   // session_present
        am::connect_reason_code::success,
        am::properties{
            am::property::topic_alias_maximum{2}
        }
    };

    ep.next_layer().set_recv_packets(
        {
            // receive packets
            connack,
        }
    );

    ep.set_auto_map_topic_alias_send(true);

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
    auto publish_t1 = am::v5::publish_packet(
        *pid_opt1,
        am::allocate_buffer("topic1"),
        am::allocate_buffer("payload1"),
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{}
    );

    auto publish_mapped_ta1 = am::v5::publish_packet(
        *pid_opt1,
        am::allocate_buffer("topic1"),
        am::buffer("payload1"),
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{
            am::property::topic_alias{1}
        }
    );

    auto publish_use_ta1 = am::v5::publish_packet(
        *pid_opt1,
        am::buffer{},
        am::buffer("payload1"),
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{
            am::property::topic_alias{1}
        }
    );

    auto pid_opt2 = ep.acquire_unique_packet_id(as::use_future).get();
    BOOST_TEST(pid_opt2.has_value());
    auto publish_t2 = am::v5::publish_packet(
        *pid_opt2,
        am::allocate_buffer("topic2"),
        am::allocate_buffer("payload1"),
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{}
    );

    auto publish_mapped_ta2 = am::v5::publish_packet(
        *pid_opt2,
        am::allocate_buffer("topic2"),
        am::buffer("payload1"),
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{
            am::property::topic_alias{2}
        }
    );

    auto publish_use_ta2 = am::v5::publish_packet(
        *pid_opt2,
        am::buffer{},
        am::buffer("payload1"),
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{
            am::property::topic_alias{2}
        }
    );

    auto pid_opt3 = ep.acquire_unique_packet_id(as::use_future).get();
    BOOST_TEST(pid_opt3.has_value());
    auto publish_t3 = am::v5::publish_packet(
        *pid_opt3,
        am::allocate_buffer("topic3"),
        am::allocate_buffer("payload1"),
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{}
    );

    auto publish_mapped_ta1_2 = am::v5::publish_packet(
        *pid_opt3,
        am::allocate_buffer("topic3"),
        am::buffer("payload1"),
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{
            am::property::topic_alias{1}
        }
    );

    auto publish_use_ta1_2 = am::v5::publish_packet(
        *pid_opt3,
        am::buffer{},
        am::buffer("payload1"),
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{
            am::property::topic_alias{1}
        }
    );

    // send publish_t1
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(publish_mapped_ta1, wp));
        }
    );
    {
        auto ec = ep.send(publish_t1, as::use_future).get();
        BOOST_TEST(!ec);
    }

    // send publish_t1
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(publish_use_ta1, wp));
        }
    );
    {
        auto ec = ep.send(publish_t1, as::use_future).get();
        BOOST_TEST(!ec);
    }

    // send publish_t2
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(publish_mapped_ta2, wp));
        }
    );
    {
        auto ec = ep.send(publish_t2, as::use_future).get();
        BOOST_TEST(!ec);
    }

    // send publish_t2
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(publish_use_ta2, wp));
        }
    );
    {
        auto ec = ep.send(publish_t2, as::use_future).get();
        BOOST_TEST(!ec);
    }

    // send publish_t3
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(publish_mapped_ta1_2, wp));
        }
    );
    {
        auto ec = ep.send(publish_t3, as::use_future).get();
        BOOST_TEST(!ec);
    }

    // send publish_t3
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(publish_use_ta1_2, wp));
        }
    );
    {
        auto ec = ep.send(publish_t3, as::use_future).get();
        BOOST_TEST(!ec);
    }
    ep.close(as::use_future).get();
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

    using ep_t = am::endpoint<async_mqtt::role::client, async_mqtt::stub_socket>;
    ep_t ep{
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
        true,   // session_present
        am::connect_reason_code::success,
        am::properties{
            am::property::topic_alias_maximum{2}
        }
    };

    ep.next_layer().set_recv_packets(
        {
            // receive packets
            connack,
        }
    );

    ep.set_auto_replace_topic_alias_send(true);

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
    auto publish_t1 = am::v5::publish_packet(
        *pid_opt1,
        am::allocate_buffer("topic1"),
        am::allocate_buffer("payload1"),
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{}
    );

    auto pid_opt2 = ep.acquire_unique_packet_id(as::use_future).get();
    BOOST_TEST(pid_opt2.has_value());
    auto publish_map_ta1 = am::v5::publish_packet(
        *pid_opt2,
        am::allocate_buffer("topic1"),
        am::buffer("payload1"),
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{
            am::property::topic_alias{1}
        }
    );

    auto pid_opt3 = ep.acquire_unique_packet_id(as::use_future).get();
    BOOST_TEST(pid_opt3.has_value());
    auto publish_use_ta1 = am::v5::publish_packet(
        *pid_opt3,
        am::buffer{},
        am::buffer("payload1"),
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{
            am::property::topic_alias{1}
        }
    );

    auto pid_opt4 = ep.acquire_unique_packet_id(as::use_future).get();
    BOOST_TEST(pid_opt4.has_value());
    auto publish_t1_2 = am::v5::publish_packet(
        *pid_opt4,
        am::allocate_buffer("topic1"),
        am::allocate_buffer("payload1"),
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{}
    );

    auto publish_exp_ta1 = am::v5::publish_packet(
        *pid_opt4,
        am::buffer{},
        am::buffer("payload1"),
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{
            am::property::topic_alias{1}
        }
    );

    // send publish_t1
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(publish_t1, wp));
        }
    );
    {
        auto ec = ep.send(publish_t1, as::use_future).get();
        BOOST_TEST(!ec);
    }

    // send publish_use_ta1
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(publish_use_ta1, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::bad_message);
    }

    // send publish_map_ta1
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(publish_map_ta1, wp));
        }
    );
    {
        auto ec = ep.send(publish_map_ta1, as::use_future).get();
        BOOST_TEST(!ec);
    }

    // send publish_t1
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            // auto use
            BOOST_TEST(am::packet_compare(publish_exp_ta1, wp));
        }
    );
    {
        auto ec = ep.send(publish_t1_2, as::use_future).get();
        BOOST_TEST(!ec);
    }
    ep.close(as::use_future).get();
    guard.reset();
    th.join();
}

BOOST_AUTO_TEST_CASE(recv_client) {
    auto version = am::protocol_version::v5;
    as::io_context ioc;
    auto guard = as::make_work_guard(ioc.get_executor());
    std::thread th {
        [&] {
            ioc.run();
        }
    };

    using ep_t = am::endpoint<async_mqtt::role::client, async_mqtt::stub_socket>;
    ep_t ep{
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
            am::property::topic_alias_maximum{2}
        }
    };

    auto connack = am::v5::connack_packet{
        true,   // session_present
        am::connect_reason_code::success,
        am::properties{}
    };

    // internal
    auto disconnect = am::v5::disconnect_packet{
        am::disconnect_reason_code::topic_alias_invalid
    };

    auto close = am::make_error(am::errc::network_reset, "pseudo close");

    ep.next_layer().set_recv_packets(
        {
            // receive packets
            connack,
        }
    );

    auto init =
        [&] {
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
        };

    init();

    auto pid_opt1 = ep.acquire_unique_packet_id(as::use_future).get();
    BOOST_TEST(pid_opt1.has_value());
    auto publish_reg_t1 = am::v5::publish_packet(
        *pid_opt1,
        am::allocate_buffer("topic1"),
        am::allocate_buffer("payload1"),
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{
            am::property::topic_alias{1}
        }
    );

    auto pid_opt2 = ep.acquire_unique_packet_id(as::use_future).get();
    BOOST_TEST(pid_opt2.has_value());
    auto publish_use_ta1 = am::v5::publish_packet(
        *pid_opt2,
        am::buffer{},
        am::buffer("payload1"),
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{
            am::property::topic_alias{1}
        }
    );

    auto publish_exp_t1 = am::v5::publish_packet(
        *pid_opt2,
        am::allocate_buffer("topic1"),
        am::allocate_buffer("payload1"),
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{
            am::property::topic_alias{1}
        }
    );

    auto pid_opt3 = ep.acquire_unique_packet_id(as::use_future).get();
    BOOST_TEST(pid_opt3.has_value());
    auto publish_reg_t2 = am::v5::publish_packet(
        *pid_opt3,
        am::allocate_buffer("topic2"),
        am::allocate_buffer("payload1"),
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{
            am::property::topic_alias{2}
        }
    );

    auto pid_opt4 = ep.acquire_unique_packet_id(as::use_future).get();
    BOOST_TEST(pid_opt4.has_value());
    auto publish_use_ta2 = am::v5::publish_packet(
        *pid_opt4,
        am::buffer{},
        am::buffer("payload1"),
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{
            am::property::topic_alias{2}
        }
    );

    auto publish_exp_t2 = am::v5::publish_packet(
        *pid_opt4,
        am::allocate_buffer("topic2"),
        am::allocate_buffer("payload1"),
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{
            am::property::topic_alias{2}
        }
    );

    auto pid_opt5 = ep.acquire_unique_packet_id(as::use_future).get();
    BOOST_TEST(pid_opt5.has_value());
    auto publish_reg_t3 = am::v5::publish_packet(
        *pid_opt5,
        am::allocate_buffer("topic3"),
        am::allocate_buffer("payload1"),
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{
            am::property::topic_alias{3} // over
        }
    );

    auto pid_opt6 = ep.acquire_unique_packet_id(as::use_future).get();
    BOOST_TEST(pid_opt6.has_value());
    auto publish_use_ta3 = am::v5::publish_packet(
        *pid_opt6,
        am::buffer{},
        am::buffer("payload1"),
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{
            am::property::topic_alias{3} // over
        }
    );

    auto pid_opt7 = ep.acquire_unique_packet_id(as::use_future).get();
    BOOST_TEST(pid_opt7.has_value());
    auto publish_upd_t3 = am::v5::publish_packet(
        *pid_opt7,
        am::allocate_buffer("topic3"),
        am::allocate_buffer("payload1"),
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{
            am::property::topic_alias{1} // update
        }
    );

    auto pid_opt8 = ep.acquire_unique_packet_id(as::use_future).get();
    BOOST_TEST(pid_opt8.has_value());
    auto publish_use_ta1_t3 = am::v5::publish_packet(
        *pid_opt8,
        am::buffer{},
        am::buffer("payload1"),
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{
            am::property::topic_alias{1}
        }
    );

    auto publish_exp_t3 = am::v5::publish_packet(
        *pid_opt8,
        am::allocate_buffer("topic3"),
        am::allocate_buffer("payload1"),
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{
            am::property::topic_alias{1} // update
        }
    );

    ep.next_layer().set_recv_packets(
        {
            // receive packets
            publish_reg_t1,
            publish_use_ta1,
            publish_reg_t2,
            publish_use_ta2,
            publish_reg_t3,  // error and disconnect
            close,
            connack,
            publish_use_ta3, // error and disconnect
            close,
            connack,
            publish_upd_t3,
            publish_use_ta1_t3,
        }
    );

    // recv publish_reg_t1
    {
        auto pv = ep.recv(as::use_future).get();
        BOOST_TEST(am::packet_compare(publish_reg_t1, pv));
    }

    // recv publish_use_ta1
    {
        auto pv = ep.recv(as::use_future).get();
        BOOST_TEST(am::packet_compare(publish_exp_t1, pv));
    }

    // recv publish_reg_t2
    {
        auto pv = ep.recv(as::use_future).get();
        BOOST_TEST(am::packet_compare(publish_reg_t2, pv));
    }

    // recv publish_use_ta2
    {
        auto pv = ep.recv(as::use_future).get();
        BOOST_TEST(am::packet_compare(publish_exp_t2, pv));
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
    // recv publish_reg_t3 (invalid)
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

    init();

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
    // recv publish_use_ta3
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

    init();
    // recv publish_upd_t3
    {
        auto pv = ep.recv(as::use_future).get();
        BOOST_TEST(am::packet_compare(publish_upd_t3, pv));
    }

    // recv publish_use_ta1
    {
        auto pv = ep.recv(as::use_future).get();
        BOOST_TEST(am::packet_compare(publish_exp_t3, pv));
    }
    ep.close(as::use_future).get();
    guard.reset();
    th.join();
}

BOOST_AUTO_TEST_CASE(recv_server) {
    auto version = am::protocol_version::v5;
    as::io_context ioc;
    auto guard = as::make_work_guard(ioc.get_executor());
    std::thread th {
        [&] {
            ioc.run();
        }
    };

    using ep_t = am::endpoint<async_mqtt::role::server, async_mqtt::stub_socket>;
    ep_t ep{
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
        true,   // session_present
        am::connect_reason_code::success,
        am::properties{
            am::property::topic_alias_maximum{2}
        }
    };

    // internal
    auto disconnect = am::v5::disconnect_packet{
        am::disconnect_reason_code::topic_alias_invalid
    };

    auto close = am::make_error(am::errc::network_reset, "pseudo close");

    ep.next_layer().set_recv_packets(
        {
            // receive packets
            connect,
        }
    );

    auto init =
        [&] {
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

        };

    init();

    auto pid_opt1 = ep.acquire_unique_packet_id(as::use_future).get();
    BOOST_TEST(pid_opt1.has_value());
    auto publish_reg_t1 = am::v5::publish_packet(
        *pid_opt1,
        am::allocate_buffer("topic1"),
        am::allocate_buffer("payload1"),
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{
            am::property::topic_alias{1}
        }
    );

    auto pid_opt2 = ep.acquire_unique_packet_id(as::use_future).get();
    BOOST_TEST(pid_opt2.has_value());
    auto publish_use_ta1 = am::v5::publish_packet(
        *pid_opt2,
        am::buffer{},
        am::buffer("payload1"),
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{
            am::property::topic_alias{1}
        }
    );

    auto publish_exp_t1 = am::v5::publish_packet(
        *pid_opt2,
        am::allocate_buffer("topic1"),
        am::allocate_buffer("payload1"),
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{
            am::property::topic_alias{1}
        }
    );

    auto pid_opt3 = ep.acquire_unique_packet_id(as::use_future).get();
    BOOST_TEST(pid_opt3.has_value());
    auto publish_reg_t2 = am::v5::publish_packet(
        *pid_opt3,
        am::allocate_buffer("topic2"),
        am::allocate_buffer("payload1"),
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{
            am::property::topic_alias{2}
        }
    );

    auto pid_opt4 = ep.acquire_unique_packet_id(as::use_future).get();
    BOOST_TEST(pid_opt4.has_value());
    auto publish_use_ta2 = am::v5::publish_packet(
        *pid_opt4,
        am::buffer{},
        am::buffer("payload1"),
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{
            am::property::topic_alias{2}
        }
    );

    auto publish_exp_t2 = am::v5::publish_packet(
        *pid_opt4,
        am::allocate_buffer("topic2"),
        am::allocate_buffer("payload1"),
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{
            am::property::topic_alias{2}
        }
    );

    auto pid_opt5 = ep.acquire_unique_packet_id(as::use_future).get();
    BOOST_TEST(pid_opt5.has_value());
    auto publish_reg_t3 = am::v5::publish_packet(
        *pid_opt5,
        am::allocate_buffer("topic3"),
        am::allocate_buffer("payload1"),
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{
            am::property::topic_alias{3} // over
        }
    );

    auto pid_opt6 = ep.acquire_unique_packet_id(as::use_future).get();
    BOOST_TEST(pid_opt6.has_value());
    auto publish_use_ta3 = am::v5::publish_packet(
        *pid_opt6,
        am::buffer{},
        am::buffer("payload1"),
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{
            am::property::topic_alias{3} // over
        }
    );

    auto pid_opt7 = ep.acquire_unique_packet_id(as::use_future).get();
    BOOST_TEST(pid_opt7.has_value());
    auto publish_upd_t3 = am::v5::publish_packet(
        *pid_opt7,
        am::allocate_buffer("topic3"),
        am::allocate_buffer("payload1"),
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{
            am::property::topic_alias{1} // update
        }
    );

    auto pid_opt8 = ep.acquire_unique_packet_id(as::use_future).get();
    BOOST_TEST(pid_opt8.has_value());
    auto publish_use_ta1_t3 = am::v5::publish_packet(
        *pid_opt8,
        am::buffer{},
        am::buffer("payload1"),
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{
            am::property::topic_alias{1}
        }
    );

    auto publish_exp_t3 = am::v5::publish_packet(
        *pid_opt8,
        am::allocate_buffer("topic3"),
        am::allocate_buffer("payload1"),
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{
            am::property::topic_alias{1} // update
        }
    );

    ep.next_layer().set_recv_packets(
        {
            // receive packets
            publish_reg_t1,
            publish_use_ta1,
            publish_reg_t2,
            publish_use_ta2,
            publish_reg_t3,  // error and disconnect
            close,
            connect,
            publish_use_ta3, // error and disconnect
            close,
            connect,
            publish_upd_t3,
            publish_use_ta1_t3,
        }
    );

    // recv publish_reg_t1
    {
        auto pv = ep.recv(as::use_future).get();
        BOOST_TEST(am::packet_compare(publish_reg_t1, pv));
    }

    // recv publish_use_ta1
    {
        auto pv = ep.recv(as::use_future).get();
        BOOST_TEST(am::packet_compare(publish_exp_t1, pv));
    }

    // recv publish_reg_t2
    {
        auto pv = ep.recv(as::use_future).get();
        BOOST_TEST(am::packet_compare(publish_reg_t2, pv));
    }

    // recv publish_use_ta2
    {
        auto pv = ep.recv(as::use_future).get();
        BOOST_TEST(am::packet_compare(publish_exp_t2, pv));
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
    // recv publish_reg_t3 (invalid)
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

    init();
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
    // recv publish_use_ta3
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

    init();
    // recv publish_upd_t3
    {
        auto pv = ep.recv(as::use_future).get();
        BOOST_TEST(am::packet_compare(publish_upd_t3, pv));
    }

    // recv publish_use_ta1
    {
        auto pv = ep.recv(as::use_future).get();
        BOOST_TEST(am::packet_compare(publish_exp_t3, pv));
    }

    ep.close(as::use_future).get();
    guard.reset();
    th.join();
}

BOOST_AUTO_TEST_SUITE_END()
