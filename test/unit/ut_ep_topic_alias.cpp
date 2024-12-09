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

BOOST_AUTO_TEST_SUITE(ut_ep_topic_alias)

namespace am = async_mqtt;
namespace as = boost::asio;

inline std::optional<am::topic_alias_type> get_topic_alias(am::properties const& props) {
    std::optional<am::topic_alias_type> ta_opt;
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
    auto ep = ep_t{
        version,
        // for stub_socket args
        version,
        ioc.get_executor()
    };

    auto connect = am::v5::connect_packet{
        true,   // clean_start
        0x1234, // keep_alive
        "cid1",
        std::nullopt, // will
        "user1",
        "pass1",
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
            {connack},
        }
    );

    // underlying handshake
    {
        auto [ec] = ep.async_underlying_handshake(as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
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

    // recv connack
    {
        auto [ec, pv] = ep.async_recv(as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
        BOOST_TEST(connack == *pv);
    }

    auto [ec1, pid1] = ep.async_acquire_unique_packet_id(as::as_tuple(as::use_future)).get();
    BOOST_TEST(!ec1);
    BOOST_TEST(pid1 != 0);
    auto publish_reg_t1 = am::v5::publish_packet(
        pid1,
        "topic1",
        "payload1",
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{
            am::property::topic_alias{1}
        }
    );

    auto [ec2, pid2] = ep.async_acquire_unique_packet_id(as::as_tuple(as::use_future)).get();
    BOOST_TEST(!ec2);
    BOOST_TEST(pid2 != 0);
    auto publish_use_ta1 = am::v5::publish_packet(
        pid2,
        "",
        "payload1",
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{
            am::property::topic_alias{1}
        }
    );

    auto [ec3, pid3] = ep.async_acquire_unique_packet_id(as::as_tuple(as::use_future)).get();
    BOOST_TEST(!ec3);
    BOOST_TEST(pid3 != 0);
    auto publish_reg_t2 = am::v5::publish_packet(
        pid3,
        "topic2",
        "payload1",
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{
            am::property::topic_alias{2}
        }
    );

    auto [ec4, pid4] = ep.async_acquire_unique_packet_id(as::as_tuple(as::use_future)).get();
    BOOST_TEST(!ec4);
    BOOST_TEST(pid4 != 0);
    auto publish_use_ta2 = am::v5::publish_packet(
        pid4,
        "",
        "payload1",
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{
            am::property::topic_alias{2}
        }
    );

    auto [ec5, pid5] = ep.async_acquire_unique_packet_id(as::as_tuple(as::use_future)).get();
    BOOST_TEST(!ec5);
    BOOST_TEST(pid5 != 0);
    auto publish_reg_t3 = am::v5::publish_packet(
        pid5,
        "topic3",
        "payload1",
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{
            am::property::topic_alias{3} // over
        }
    );

    auto [ec6, pid6] = ep.async_acquire_unique_packet_id(as::as_tuple(as::use_future)).get();
    BOOST_TEST(!ec6);
    BOOST_TEST(pid6 != 0);
    auto publish_use_ta3 = am::v5::publish_packet(
        pid6,
        "",
        "payload1",
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{
            am::property::topic_alias{3} // over
        }
    );

    auto [ec7, pid7] = ep.async_acquire_unique_packet_id(as::as_tuple(as::use_future)).get();
    BOOST_TEST(!ec7);
    BOOST_TEST(pid7 != 0);
    auto publish_upd_t3 = am::v5::publish_packet(
        pid7,
        "topic3",
        "payload1",
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{
            am::property::topic_alias{1} // update
        }
    );

    // send publish_reg_t1
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(publish_reg_t1 == wp);
        }
    );
    {
        auto [ec] = ep.async_send(publish_reg_t1, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }
#if !defined(_MSC_VER) || _MSC_VER >= 1930
    {   // check regulate
        auto p1 = publish_reg_t1;
        auto [ec1, rp1] = ep.async_regulate_for_store(p1, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec1);
        BOOST_TEST(rp1.topic() == "topic1");
        BOOST_TEST(!get_topic_alias(rp1.props()));

        // idempotence
        auto p2 = p1;
        auto [ec2, rp2] = ep.async_regulate_for_store(p2, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec2);
        BOOST_TEST(rp2.topic() == "topic1");
        BOOST_TEST(!get_topic_alias(rp2.props()));
    }
#endif // !defined(_MSC_VER) || _MSC_VER >= 1930

    // send publish_use_ta1
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(publish_use_ta1 == wp);
        }
    );
    {
        auto [ec] = ep.async_send(publish_use_ta1, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }
#if !defined(_MSC_VER) || _MSC_VER >= 1930
    {   // check regulate
        auto p = publish_use_ta1;
        auto [ec, rp] = ep.async_regulate_for_store(p, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
        BOOST_TEST(rp.topic() == "topic1");
        BOOST_TEST(!get_topic_alias(rp.props()));
    }
#endif // !defined(_MSC_VER) || _MSC_VER >= 1930

    {
        // sync version
        std::promise<void> pro;
        auto fut = pro.get_future();
        as::dispatch(
            as::bind_executor(
                ioc.get_executor(),
                [&] {
                    am::error_code ec;
                    auto publish_not_reg_ta = am::v5::publish_packet(
                        0,
                        "",
                        "payload1",
                        am::qos::at_most_once | am::pub::retain::no | am::pub::dup::no,
                        am::properties{
                            am::property::topic_alias{999}
                        }
                    );
                    ep.regulate_for_store(publish_not_reg_ta, ec);
                    BOOST_TEST(ec == am::mqtt_error::packet_not_regulated);
                    pro.set_value();
                }
            )
        );
        fut.get();
    }
    {
        // sync version
        std::promise<void> pro;
        auto fut = pro.get_future();
        as::dispatch(
            as::bind_executor(
                ioc.get_executor(),
                [&] {
                    am::error_code ec;
                    auto publish_no_ta = am::v5::publish_packet(
                        0,
                        "",
                        "payload1",
                        am::qos::at_most_once | am::pub::retain::no | am::pub::dup::no
                    );
                    ep.regulate_for_store(publish_no_ta, ec);
                    BOOST_TEST(ec == am::mqtt_error::packet_not_regulated);
                    pro.set_value();
                }
            )
        );
        fut.get();
    }

    // send publish_reg_t2
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(publish_reg_t2 == wp);
        }
    );
    {
        auto [ec] = ep.async_send(publish_reg_t2, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }

    // send publish_use_ta2
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(publish_use_ta2 == wp);
        }
    );
    {
        auto [ec] = ep.async_send(publish_use_ta2, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }

    // send publish_reg_t3
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(publish_reg_t3, as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

    // send publish_use_ta3
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(publish_use_ta3, as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

    // send publish_upd_t3
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(publish_upd_t3 == wp);
        }
    );
    {
        auto [ec] = ep.async_send(publish_upd_t3, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }
    ep.async_close(as::as_tuple(as::use_future)).get();
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
    auto ep = ep_t{
        version,
        // for stub_socket args
        version,
        ioc.get_executor()
    };

    auto connect = am::v5::connect_packet{
        true,   // clean_start
        0x1234, // keep_alive
        "cid1",
        std::nullopt, // will
        "user1",
        "pass1",
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
            {connect},
        }
    );

    // connection established as server
    ep.underlying_accepted();
    // recv connect
    {
        auto [ec, pv] = ep.async_recv(as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
        BOOST_TEST(connect == *pv);
    }

    // send connack
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(connack == wp);
        }
    );
    {
        auto [ec] = ep.async_send(connack, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }

    auto [ec1, pid1] = ep.async_acquire_unique_packet_id(as::as_tuple(as::use_future)).get();
    BOOST_TEST(!ec1);
    BOOST_TEST(pid1 != 0);
    auto publish_reg_t1 = am::v5::publish_packet(
        pid1,
        "topic1",
        "payload1",
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{
            am::property::topic_alias{1}
        }
    );

    auto [ec2, pid2] = ep.async_acquire_unique_packet_id(as::as_tuple(as::use_future)).get();
    BOOST_TEST(!ec2);
    BOOST_TEST(pid2 != 0);
    auto publish_use_ta1 = am::v5::publish_packet(
        pid2,
        "",
        "payload1",
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{
            am::property::topic_alias{1}
        }
    );

    auto [ec3, pid3] = ep.async_acquire_unique_packet_id(as::as_tuple(as::use_future)).get();
    BOOST_TEST(!ec3);
    BOOST_TEST(pid3 != 0);
    auto publish_reg_t2 = am::v5::publish_packet(
        pid3,
        "topic2",
        "payload1",
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{
            am::property::topic_alias{2}
        }
    );

    auto [ec4, pid4] = ep.async_acquire_unique_packet_id(as::as_tuple(as::use_future)).get();
    BOOST_TEST(!ec4);
    BOOST_TEST(pid4 != 0);
    auto publish_use_ta2 = am::v5::publish_packet(
        pid4,
        "",
        "payload1",
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{
            am::property::topic_alias{2}
        }
    );

    auto [ec5, pid5] = ep.async_acquire_unique_packet_id(as::as_tuple(as::use_future)).get();
    BOOST_TEST(!ec5);
    BOOST_TEST(pid5 != 0);
    auto publish_reg_t3 = am::v5::publish_packet(
        pid5,
        "topic3",
        "payload1",
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{
            am::property::topic_alias{3} // over
        }
    );

    auto [ec6, pid6] = ep.async_acquire_unique_packet_id(as::as_tuple(as::use_future)).get();
    BOOST_TEST(!ec6);
    BOOST_TEST(pid6 != 0);
    auto publish_use_ta3 = am::v5::publish_packet(
        pid6,
        "",
        "payload1",
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{
            am::property::topic_alias{3} // over
        }
    );

    auto [ec7, pid7] = ep.async_acquire_unique_packet_id(as::as_tuple(as::use_future)).get();
    BOOST_TEST(!ec7);
    BOOST_TEST(pid7 != 0);
    auto publish_upd_t3 = am::v5::publish_packet(
        pid7,
        "topic3",
        "payload1",
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{
            am::property::topic_alias{1} // update
        }
    );

    // send publish_reg_t1
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(publish_reg_t1 == wp);
        }
    );
    {
        auto [ec] = ep.async_send(publish_reg_t1, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }

    // send publish_use_ta1
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(publish_use_ta1 == wp);
        }
    );
    {
        auto [ec] = ep.async_send(publish_use_ta1, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }

    // send publish_reg_t2
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(publish_reg_t2 == wp);
        }
    );
    {
        auto [ec] = ep.async_send(publish_reg_t2, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }

    // send publish_use_ta2
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(publish_use_ta2 == wp);
        }
    );
    {
        auto [ec] = ep.async_send(publish_use_ta2, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }

    // send publish_reg_t3
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(publish_reg_t3, as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

    // send publish_use_ta3
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(publish_use_ta3, as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

    // send publish_upd_t3
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(publish_upd_t3 == wp);
        }
    );
    {
        auto [ec] = ep.async_send(publish_upd_t3, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }
    ep.async_close(as::as_tuple(as::use_future)).get();
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
    auto ep = ep_t{
        version,
        // for stub_socket args
        version,
        ioc.get_executor()
    };

    auto connect = am::v5::connect_packet{
        true,   // clean_start
        0x1234, // keep_alive
        "cid1",
        std::nullopt, // will
        "user1",
        "pass1",
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
            {connack},
        }
    );

    ep.set_auto_map_topic_alias_send(true);

    // underlying handshake
    {
        auto [ec] = ep.async_underlying_handshake(as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
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

    // recv connack
    {
        auto [ec, pv] = ep.async_recv(as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
        BOOST_TEST(connack == *pv);
    }

    auto [ec1, pid1] = ep.async_acquire_unique_packet_id(as::as_tuple(as::use_future)).get();
    BOOST_TEST(!ec1);
    BOOST_TEST(pid1 != 0);
    auto publish_t1 = am::v5::publish_packet(
        pid1,
        "topic1",
        "payload1",
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{}
    );

    auto publish_mapped_ta1 = am::v5::publish_packet(
        pid1,
        "topic1",
        "payload1",
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{
            am::property::topic_alias{1}
        }
    );

    auto publish_use_ta1 = am::v5::publish_packet(
        pid1,
        "",
        "payload1",
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{
            am::property::topic_alias{1}
        }
    );

    auto [ec2, pid2] = ep.async_acquire_unique_packet_id(as::as_tuple(as::use_future)).get();
    BOOST_TEST(!ec2);
    BOOST_TEST(pid2 != 0);
    auto publish_t2 = am::v5::publish_packet(
        pid2,
        "topic2",
        "payload1",
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{}
    );

    auto publish_mapped_ta2 = am::v5::publish_packet(
        pid2,
        "topic2",
        "payload1",
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{
            am::property::topic_alias{2}
        }
    );

    auto publish_use_ta2 = am::v5::publish_packet(
        pid2,
        "",
        "payload1",
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{
            am::property::topic_alias{2}
        }
    );

    auto [ec3, pid3] = ep.async_acquire_unique_packet_id(as::as_tuple(as::use_future)).get();
    BOOST_TEST(!ec3);
    BOOST_TEST(pid3 != 0);
    auto publish_t3 = am::v5::publish_packet(
        pid3,
        "topic3",
        "payload1",
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{}
    );

    auto publish_mapped_ta1_2 = am::v5::publish_packet(
        pid3,
        "topic3",
        "payload1",
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{
            am::property::topic_alias{1}
        }
    );

    auto publish_use_ta1_2 = am::v5::publish_packet(
        pid3,
        "",
        "payload1",
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{
            am::property::topic_alias{1}
        }
    );

    // send publish_t1
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(publish_mapped_ta1 == wp);
        }
    );
    {
        auto [ec] = ep.async_send(publish_t1, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }

    // send publish_t1
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(publish_use_ta1 == wp);
        }
    );
    {
        auto [ec] = ep.async_send(publish_t1, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }

    // send publish_t2
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(publish_mapped_ta2 == wp);
        }
    );
    {
        auto [ec] = ep.async_send(publish_t2, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }

    // send publish_t2
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(publish_use_ta2 == wp);
        }
    );
    {
        auto [ec] = ep.async_send(publish_t2, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }

    // send publish_t3
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(publish_mapped_ta1_2 == wp);
        }
    );
    {
        auto [ec] = ep.async_send(publish_t3, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }

    // send publish_t3
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(publish_use_ta1_2 == wp);
        }
    );
    {
        auto [ec] = ep.async_send(publish_t3, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }
    ep.async_close(as::as_tuple(as::use_future)).get();
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
    auto ep = ep_t{
        version,
        // for stub_socket args
        version,
        ioc.get_executor()
    };

    auto connect = am::v5::connect_packet{
        true,   // clean_start
        0x1234, // keep_alive
        "cid1",
        std::nullopt, // will
        "user1",
        "pass1",
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
            {connack},
        }
    );

    ep.set_auto_replace_topic_alias_send(true);

    // underlying handshake
    {
        auto [ec] = ep.async_underlying_handshake(as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
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

    // recv connack
    {
        auto [ec, pv] = ep.async_recv(as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);

        BOOST_TEST(connack == *pv);
    }

    auto [ec1, pid1] = ep.async_acquire_unique_packet_id(as::as_tuple(as::use_future)).get();
    BOOST_TEST(!ec1);
    BOOST_TEST(pid1 != 0);
    auto publish_t1 = am::v5::publish_packet(
        pid1,
        "topic1",
        "payload1",
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{}
    );

    auto [ec2, pid2] = ep.async_acquire_unique_packet_id(as::as_tuple(as::use_future)).get();
    BOOST_TEST(!ec2);
    BOOST_TEST(pid2 != 0);
    auto publish_map_ta1 = am::v5::publish_packet(
        pid2,
        "topic1",
        "payload1",
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{
            am::property::topic_alias{1}
        }
    );

    auto [ec3, pid3] = ep.async_acquire_unique_packet_id(as::as_tuple(as::use_future)).get();
    BOOST_TEST(!ec3);
    BOOST_TEST(pid3 != 0);
    auto publish_use_ta1 = am::v5::publish_packet(
        pid3,
        "",
        "payload1",
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{
            am::property::topic_alias{1}
        }
    );

    auto [ec4, pid4] = ep.async_acquire_unique_packet_id(as::as_tuple(as::use_future)).get();
    BOOST_TEST(!ec4);
    BOOST_TEST(pid4 != 0);
    auto publish_t1_2 = am::v5::publish_packet(
        pid4,
        "topic1",
        "payload1",
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{}
    );

    auto publish_exp_ta1 = am::v5::publish_packet(
        pid4,
        "",
        "payload1",
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{
            am::property::topic_alias{1}
        }
    );

    // send publish_t1
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(publish_t1 == wp);
        }
    );
    {
        auto [ec] = ep.async_send(publish_t1, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }

    // send publish_use_ta1
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(publish_use_ta1, as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

    // send publish_map_ta1
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(publish_map_ta1 == wp);
        }
    );
    {
        auto [ec] = ep.async_send(publish_map_ta1, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }

    // send publish_t1
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            // auto use
            BOOST_TEST(publish_exp_ta1 == wp);
        }
    );
    {
        auto [ec] = ep.async_send(publish_t1_2, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }
    ep.async_close(as::as_tuple(as::use_future)).get();
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
    auto ep = ep_t{
        version,
        // for stub_socket args
        version,
        ioc.get_executor()
    };

    auto connect = am::v5::connect_packet{
        true,   // clean_start
        0x1234, // keep_alive
        "cid1",
        std::nullopt, // will
        "user1",
        "pass1",
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

    ep.next_layer().set_recv_packets(
        {
            // receive packets
            {connack},
        }
    );

    auto init =
        [&] {
            // underlying handshake
            {
                auto [ec] = ep.async_underlying_handshake(as::as_tuple(as::use_future)).get();
                BOOST_TEST(!ec);
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

            // recv connack
            {
                auto [ec, pv] = ep.async_recv(as::as_tuple(as::use_future)).get();
                BOOST_TEST(!ec);

                BOOST_TEST(connack == *pv);
            }
        };

    init();

    auto [ec1, pid1] = ep.async_acquire_unique_packet_id(as::as_tuple(as::use_future)).get();
    BOOST_TEST(!ec1);
    BOOST_TEST(pid1 != 0);
    auto publish_reg_t1 = am::v5::publish_packet(
        pid1,
        "topic1",
        "payload1",
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{
            am::property::topic_alias{1}
        }
    );

    auto [ec2, pid2] = ep.async_acquire_unique_packet_id(as::as_tuple(as::use_future)).get();
    BOOST_TEST(!ec2);
    BOOST_TEST(pid2 != 0);
    auto publish_use_ta1 = am::v5::publish_packet(
        pid2,
        "",
        "payload1",
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{
            am::property::topic_alias{1}
        }
    );

    auto publish_exp_t1 = am::v5::publish_packet(
        pid2,
        "topic1",
        "payload1",
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{
            am::property::topic_alias{1}
        }
    );

    auto [ec3, pid3] = ep.async_acquire_unique_packet_id(as::as_tuple(as::use_future)).get();
    BOOST_TEST(!ec3);
    BOOST_TEST(pid3 != 0);
    auto publish_reg_t2 = am::v5::publish_packet(
        pid3,
        "topic2",
        "payload1",
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{
            am::property::topic_alias{2}
        }
    );

    auto [ec4, pid4] = ep.async_acquire_unique_packet_id(as::as_tuple(as::use_future)).get();
    BOOST_TEST(!ec4);
    BOOST_TEST(pid4 != 0);
    auto publish_use_ta2 = am::v5::publish_packet(
        pid4,
        "",
        "payload1",
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{
            am::property::topic_alias{2}
        }
    );

    auto publish_exp_t2 = am::v5::publish_packet(
        pid4,
        "topic2",
        "payload1",
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{
            am::property::topic_alias{2}
        }
    );

    auto [ec5, pid5] = ep.async_acquire_unique_packet_id(as::as_tuple(as::use_future)).get();
    BOOST_TEST(!ec5);
    BOOST_TEST(pid5 != 0);
    auto publish_reg_t3 = am::v5::publish_packet(
        pid5,
        "topic3",
        "payload1",
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{
            am::property::topic_alias{3} // over
        }
    );

    auto [ec6, pid6] = ep.async_acquire_unique_packet_id(as::as_tuple(as::use_future)).get();
    BOOST_TEST(!ec6);
    BOOST_TEST(pid6 != 0);
    auto publish_use_ta3 = am::v5::publish_packet(
        pid6,
        "",
        "payload1",
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{
            am::property::topic_alias{3} // over
        }
    );

    auto [ec7, pid7] = ep.async_acquire_unique_packet_id(as::as_tuple(as::use_future)).get();
    BOOST_TEST(!ec7);
    BOOST_TEST(pid7 != 0);
    auto publish_upd_t3 = am::v5::publish_packet(
        pid7,
        "topic3",
        "payload1",
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{
            am::property::topic_alias{1} // update
        }
    );

    auto [ec8, pid8] = ep.async_acquire_unique_packet_id(as::as_tuple(as::use_future)).get();
    BOOST_TEST(!ec8);
    BOOST_TEST(pid8 != 0);
    auto publish_use_ta1_t3 = am::v5::publish_packet(
        pid8,
        "",
        "payload1",
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{
            am::property::topic_alias{1}
        }
    );

    auto publish_exp_t3 = am::v5::publish_packet(
        pid8,
        "topic3",
        "payload1",
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{
            am::property::topic_alias{1} // update
        }
    );

    auto [ec9, pid9] = ep.async_acquire_unique_packet_id(as::as_tuple(as::use_future)).get();
    BOOST_TEST(!ec9);
    BOOST_TEST(pid9 != 0);
    auto publish_reg_t1_again = am::v5::publish_packet(
        pid9,
        "topic1",
        "payload1",
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{
            am::property::topic_alias{1}
        }
    );

    ep.next_layer().set_recv_packets(
        {
            // receive packets
            {publish_reg_t1},
            {publish_use_ta1},
            {publish_reg_t2},
            {publish_use_ta2},
            {publish_reg_t3},  // error and disconnect

            {connack},
            {publish_use_ta3}, // error and disconnect
            {am::errc::make_error_code(am::errc::connection_reset)},
            {connack},
            {publish_reg_t1_again},
            {publish_upd_t3},
            {publish_use_ta1_t3},
        }
    );

    // recv publish_reg_t1
    {
        auto [ec, pv] = ep.async_recv(as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);

        BOOST_TEST(publish_reg_t1 == *pv);
    }

    // recv publish_use_ta1
    {
        auto [ec, pv] = ep.async_recv(as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);

        BOOST_TEST(publish_exp_t1 == *pv);
    }

    // recv publish_reg_t2
    {
        auto [ec, pv] = ep.async_recv(as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);

        BOOST_TEST(publish_reg_t2 == *pv);
    }

    // recv publish_use_ta2
    {
        auto [ec, pv] = ep.async_recv(as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);

        BOOST_TEST(publish_exp_t2 == *pv);
    }


    bool close_called = false;
    ep.next_layer().set_close_checker(
        [&] { close_called = true; }
    );
    // internal auto send disconnect
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(!close_called);
            BOOST_TEST(disconnect == wp);
        }
    );
    // recv publish_reg_t3 (invalid)
    {
        auto [ec, pv] = ep.async_recv(as::as_tuple(as::use_future)).get();
        BOOST_TEST(!pv);
        BOOST_TEST(ec == am::disconnect_reason_code::topic_alias_invalid);
    }
    BOOST_TEST(close_called);

    init();

    close_called = false;
    ep.next_layer().set_close_checker(
        [&] { close_called = true; }
    );
    // internal auto send disconnect
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(!close_called);
            BOOST_TEST(disconnect == wp);
        }
    );
    // recv publish_use_ta3
    {
        auto [ec, pv] = ep.async_recv(as::as_tuple(as::use_future)).get();
        BOOST_TEST(!pv);
        BOOST_TEST(ec == am::disconnect_reason_code::topic_alias_invalid);
    }
    BOOST_TEST(close_called);

    // recv close
    {
        auto [ec, pv] = ep.async_recv(as::as_tuple(as::use_future)).get();
        BOOST_TEST(!pv);
        BOOST_TEST(ec == am::errc::connection_reset);
    }

    init();

    // recv publish_reg_t1
    {
        auto [ec, pv] = ep.async_recv(as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
        BOOST_TEST(publish_reg_t1_again == *pv);
    }

    // recv publish_upd_t3
    {
        auto [ec, pv] = ep.async_recv(as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
        BOOST_TEST(publish_upd_t3 == *pv);
    }

    // recv publish_use_ta1
    {
        auto [ec, pv] = ep.async_recv(as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
        BOOST_TEST(publish_exp_t3 == *pv);
    }
    ep.async_close(as::as_tuple(as::use_future)).get();
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
    auto ep = ep_t{
        version,
        // for stub_socket args
        version,
        ioc.get_executor()
    };

    auto connect = am::v5::connect_packet{
        true,   // clean_start
        0x1234, // keep_alive
        "cid1",
        std::nullopt, // will
        "user1",
        "pass1",
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

    ep.next_layer().set_recv_packets(
        {
            // receive packets
            {connect},
        }
    );

    auto init =
        [&] {
            // connection established as server
            ep.underlying_accepted();
            // recv connect
            {
                auto [ec, pv] = ep.async_recv(as::as_tuple(as::use_future)).get();
                BOOST_TEST(!ec);
                BOOST_TEST(connect == *pv);
            }

            // send connack
            ep.next_layer().set_write_packet_checker(
                [&](am::packet_variant wp) {
                    BOOST_TEST(connack == wp);
                }
            );
            {
                auto [ec] = ep.async_send(connack, as::as_tuple(as::use_future)).get();
                BOOST_TEST(!ec);
            }

        };

    init();

    auto [ec1, pid1] = ep.async_acquire_unique_packet_id(as::as_tuple(as::use_future)).get();
    BOOST_TEST(!ec1);
    BOOST_TEST(pid1 != 0);
    auto publish_reg_t1 = am::v5::publish_packet(
        pid1,
        "topic1",
        "payload1",
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{
            am::property::topic_alias{1}
        }
    );

    auto [ec2, pid2] = ep.async_acquire_unique_packet_id(as::as_tuple(as::use_future)).get();
    BOOST_TEST(!ec2);
    BOOST_TEST(pid2 != 0);
    auto publish_use_ta1 = am::v5::publish_packet(
        pid2,
        "",
        "payload1",
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{
            am::property::topic_alias{1}
        }
    );

    auto publish_exp_t1 = am::v5::publish_packet(
        pid2,
        "topic1",
        "payload1",
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{
            am::property::topic_alias{1}
        }
    );

    auto [ec3, pid3] = ep.async_acquire_unique_packet_id(as::as_tuple(as::use_future)).get();
    BOOST_TEST(!ec3);
    BOOST_TEST(pid3 != 0);
    auto publish_reg_t2 = am::v5::publish_packet(
        pid3,
        "topic2",
        "payload1",
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{
            am::property::topic_alias{2}
        }
    );

    auto [ec4, pid4] = ep.async_acquire_unique_packet_id(as::as_tuple(as::use_future)).get();
    BOOST_TEST(!ec4);
    BOOST_TEST(pid4 != 0);
    auto publish_use_ta2 = am::v5::publish_packet(
        pid4,
        "",
        "payload1",
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{
            am::property::topic_alias{2}
        }
    );

    auto publish_exp_t2 = am::v5::publish_packet(
        pid4,
        "topic2",
        "payload1",
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{
            am::property::topic_alias{2}
        }
    );

    auto [ec5, pid5] = ep.async_acquire_unique_packet_id(as::as_tuple(as::use_future)).get();
    BOOST_TEST(!ec5);
    BOOST_TEST(pid5 != 0);
    auto publish_reg_t3 = am::v5::publish_packet(
        pid5,
        "topic3",
        "payload1",
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{
            am::property::topic_alias{3} // over
        }
    );

    auto [ec6, pid6] = ep.async_acquire_unique_packet_id(as::as_tuple(as::use_future)).get();
    BOOST_TEST(!ec6);
    BOOST_TEST(pid6 != 0);
    auto publish_use_ta3 = am::v5::publish_packet(
        pid6,
        "",
        "payload1",
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{
            am::property::topic_alias{3} // over
        }
    );

    auto [ec7, pid7] = ep.async_acquire_unique_packet_id(as::as_tuple(as::use_future)).get();
    BOOST_TEST(!ec7);
    BOOST_TEST(pid7 != 0);
    auto publish_upd_t3 = am::v5::publish_packet(
        pid7,
        "topic3",
        "payload1",
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{
            am::property::topic_alias{1} // update
        }
    );

    auto [ec8, pid8] = ep.async_acquire_unique_packet_id(as::as_tuple(as::use_future)).get();
    BOOST_TEST(!ec8);
    BOOST_TEST(pid8 != 0);
    auto publish_use_ta1_t3 = am::v5::publish_packet(
        pid8,
        "",
        "payload1",
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{
            am::property::topic_alias{1}
        }
    );

    auto publish_exp_t3 = am::v5::publish_packet(
        pid8,
        "topic3",
        "payload1",
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{
            am::property::topic_alias{1} // update
        }
    );

    ep.next_layer().set_recv_packets(
        {
            // receive packets
            {publish_reg_t1},
            {publish_use_ta1},
            {publish_reg_t2},
            {publish_use_ta2},
            {publish_reg_t3},  // error and disconnect
            {am::errc::make_error_code(am::errc::connection_reset)},
            {connect},
            {publish_use_ta3}, // error and disconnect
            {am::errc::make_error_code(am::errc::connection_reset)},
            {connect},
            {publish_upd_t3},
            {publish_use_ta1_t3},
        }
    );

    // recv publish_reg_t1
    {
        auto [ec, pv] = ep.async_recv(as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
        BOOST_TEST(publish_reg_t1 == *pv);
    }

    // recv publish_use_ta1
    {
        auto [ec, pv] = ep.async_recv(as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
        BOOST_TEST(publish_exp_t1 == *pv);
    }

    // recv publish_reg_t2
    {
        auto [ec, pv] = ep.async_recv(as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
        BOOST_TEST(publish_reg_t2 == *pv);
    }

    // recv publish_use_ta2
    {
        auto [ec, pv] = ep.async_recv(as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
        BOOST_TEST(publish_exp_t2 == *pv);
    }

    bool close_called = false;
    ep.next_layer().set_close_checker(
        [&] { close_called = true; }
    );
    // internal auto send disconnect
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(!close_called);
            BOOST_TEST(disconnect == wp);
        }
    );
    // recv publish_reg_t3 (invalid)
    {
        auto [ec, pv] = ep.async_recv(as::as_tuple(as::use_future)).get();
        BOOST_TEST(!pv);
        BOOST_TEST(ec == am::disconnect_reason_code::topic_alias_invalid);
    }
    BOOST_TEST(close_called);
    // recv close
    {
        auto [ec, pv] = ep.async_recv(as::as_tuple(as::use_future)).get();
        BOOST_TEST(!pv);
        BOOST_TEST(ec == am::errc::connection_reset);
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
            BOOST_TEST(disconnect == wp);
        }
    );
    // recv publish_use_ta3
    {
        auto [ec, pv] = ep.async_recv(as::as_tuple(as::use_future)).get();
        BOOST_TEST(!pv);
        BOOST_TEST(ec == am::disconnect_reason_code::topic_alias_invalid);
    }
    BOOST_TEST(close_called);
    // recv close
    {
        auto [ec, pv] = ep.async_recv(as::as_tuple(as::use_future)).get();
        BOOST_TEST(!pv);
        BOOST_TEST(ec == am::errc::connection_reset);
    }

    init();
    // recv publish_upd_t3
    {
        auto [ec, pv] = ep.async_recv(as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
        BOOST_TEST(publish_upd_t3 == *pv);
    }

    // recv publish_use_ta1
    {
        auto [ec, pv] = ep.async_recv(as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
        BOOST_TEST(publish_exp_t3 == *pv);
    }

    ep.async_close(as::as_tuple(as::use_future)).get();
    guard.reset();
    th.join();
}

BOOST_AUTO_TEST_CASE(send_error) {
    auto version = am::protocol_version::v5;
    as::io_context ioc;
    auto guard = as::make_work_guard(ioc.get_executor());
    std::thread th {
        [&] {
            ioc.run();
        }
    };

    using ep_t = am::endpoint<async_mqtt::role::client, async_mqtt::stub_socket>;
    auto ep = ep_t{
        version,
        // for stub_socket args
        version,
        ioc.get_executor()
    };

    auto connect = am::v5::connect_packet{
        true,   // clean_start
        0x1234, // keep_alive
        "cid1",
        std::nullopt, // will
        "user1",
        "pass1",
        am::properties{}
    };

    auto connack = am::v5::connack_packet{
        true,   // session_present
        am::connect_reason_code::success
    };

    ep.next_layer().set_recv_packets(
        {
            // receive packets
            {connack},
        }
    );

    // underlying handshake
    {
        auto [ec] = ep.async_underlying_handshake(as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
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

    // recv connack
    {
        auto [ec, pv] = ep.async_recv(as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
        BOOST_TEST(connack == *pv);
    }

    {
        auto publish_ta_use = am::v5::publish_packet(
            "",
            "payload1",
            am::qos::at_most_once,
            am::properties{
                am::property::topic_alias{1} // max is zero
            }
        );
        auto [ec] = ep.async_send(publish_ta_use, as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }
    {
        auto publish_ta_empty = am::v5::publish_packet(
            "",
            "payload1",
            am::qos::at_most_once
        );
        auto [ec] = ep.async_send(publish_ta_empty, as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

    ep.async_close(as::as_tuple(as::use_future)).get();
    guard.reset();
    th.join();
}

BOOST_AUTO_TEST_SUITE_END()
