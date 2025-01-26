// Copyright Takatoshi Kondo 2024
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "../common/test_main.hpp"
#include "../common/global_fixture.hpp"

#include <async_mqtt/protocol/rv_connection.hpp>
#include <async_mqtt/protocol/packet/packet_iterator.hpp>

BOOST_AUTO_TEST_SUITE(ut_connection)

namespace am = async_mqtt;


BOOST_AUTO_TEST_CASE(v5_connect_connack) {
    am::rv_connection<am::role::client> c{am::protocol_version::v5};

    auto w = am::will{
        "topic1",
        "payload1",
        am::pub::retain::yes | am::qos::at_least_once,
        am::properties{
            am::property::will_delay_interval(0x0fffffff),
            am::property::content_type("json")
        }
    };

    auto props = am::properties{
        am::property::session_expiry_interval(0x0fffffff),
        am::property::user_property("mykey", "myval")
    };

    auto p = am::v5::connect_packet{
        true,   // clean_start
        0x1234, // keep_alive
        "cid1",
        w,
        "user1",
        "pass1",
        props
    };

    auto events = c.send(p);
    BOOST_TEST(events.size() == 2);
    std::visit(
        am::overload{
            [&](am::event::send const& ev) {
                BOOST_TEST(!ev.get_release_packet_id_if_send_error());
                BOOST_TEST(ev.get() == p);
            },
            [](auto const&...) {
                BOOST_TEST(false);
            }
        },
        events[0]
    );
    std::visit(
        am::overload{
            [&](am::event::timer const& ev) {
                BOOST_TEST(ev.get_op() == am::timer_op::reset);
                BOOST_TEST(ev.get_kind() == am::timer_kind::pingreq_send);
                BOOST_CHECK(ev.get_ms() == std::chrono::seconds{0x1234});
            },
            [](auto const&...) {
                BOOST_TEST(false);
            }
        },
        events[1]
    );

    char recv_connack[] {
        0x20,       // fixed_header
        0x08,       // remaining_length
        0x01,       // session_present
        char(0x87), // connect_reason_code
        0x05,       // property_length
        0x11, 0x0f, char(0xff), char(0xff), char(0xff), // session_expiry_interval
    };

    std::istringstream is{std::string{recv_connack, sizeof(recv_connack)}};
    events = c.recv(is);
    BOOST_TEST(events.size() == 1);
    {
        auto expected =
            [&] {
                auto props = am::properties{
                    am::property::session_expiry_interval(0x0fffffff)
                };
                return am::v5::connack_packet{
                    true,   // session_present
                    am::connect_reason_code::not_authorized,
                    props
                };
            } ();
        std::visit(
            am::overload{
                [&](am::event::packet_received const& ev) {
                    BOOST_TEST(ev.get() == expected);
                },
                [](auto const&...) {
                    BOOST_TEST(false);
                }
            },
            events[0]
        );
    }
}

BOOST_AUTO_TEST_CASE(v5_topic_alias_offline_fail) {
    am::rv_connection<am::role::client> c{am::protocol_version::v5};
    c.set_offline_publish(true);
    auto pid_opt = c.acquire_unique_packet_id();
    auto publish_reg_t1 = am::v5::publish_packet(
        *pid_opt,
        "topic1",
        "payload1",
        am::qos::at_least_once,
        am::properties{
            am::property::topic_alias{1}
        }
    );
    auto events = c.send(publish_reg_t1);
    BOOST_TEST(events.size() == 2);
    std::visit(
        am::overload {
            [&](am::error_code const& e) {
                BOOST_TEST(e == am::mqtt_error::packet_not_allowed_to_send);
            },
            [](auto const&) {
                BOOST_TEST(false);
            }
        },
        events[0]
    );
    std::visit(
        am::overload {
            [&](am::event::packet_id_released const& ev) {
                BOOST_TEST(ev.get() == *pid_opt);
            },
            [](auto const&) {
                BOOST_TEST(false);
            }
        },
        events[1]
    );
}

BOOST_AUTO_TEST_CASE(v5_topic_alias_size_over_resend) {
    am::rv_connection<am::role::client> c{am::protocol_version::v5};


    auto connect = am::v5::connect_packet{
        false,   // clean_start
        0, // keep_alive
        "cid1",
        std::nullopt, // will
        "user1",
        "pass1",
        am::properties{
            am::property::session_expiry_interval{am::session_never_expire}
        }
    };

    {
        auto connack = am::v5::connack_packet{
            false,   // session_present
            am::connect_reason_code::success,
            am::properties{
                am::property::maximum_packet_size{33},
                am::property::topic_alias_maximum{0xffff}
            }
        };
        c.send(connect);

        auto connack_str{am::to_string(connack.const_buffer_sequence())};
        std::istringstream is{connack_str};
        c.recv(is);
    }

    auto publish_reg_t1 = am::v5::publish_packet(
        "topic0123456789",
        "payload1",
        am::qos::at_most_once,
        am::properties{
            am::property::topic_alias{1}
        }
    );
    auto pid_opt = c.acquire_unique_packet_id();
    auto publish_use_t1 = am::v5::publish_packet(
        *pid_opt,
        "",
        "payload1",
        am::qos::at_least_once,
        am::properties{
            am::property::topic_alias{1}
        }
    );
    c.send(publish_reg_t1);
    c.send(publish_use_t1);

    c.notify_closed();

    {
        auto connack = am::v5::connack_packet{
            true,   // session_present
            am::connect_reason_code::success,
            am::properties{
                am::property::maximum_packet_size{29},
                am::property::topic_alias_maximum{0xffff}
            }
        };
        c.send(connect);

        auto connack_str{am::to_string(connack.const_buffer_sequence())};
        std::istringstream is{connack_str};
        auto events = c.recv(is);

        BOOST_TEST(events.size() == 2);
        std::visit(
            am::overload {
                [&](am::event::packet_id_released const& ev) {
                    BOOST_TEST(ev.get() == *pid_opt);
                },
                [](auto const&) {
                    BOOST_TEST(false);
                }
            },
            events[0]
        );
        std::visit(
            am::overload {
                [&](am::event::packet_received const& ev) {
                    BOOST_TEST(ev.get() == connack);
                },
                [](auto const&) {
                    BOOST_TEST(false);
                }
            },
            events[1]
        );
    }
}

BOOST_AUTO_TEST_SUITE_END()
