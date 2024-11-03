// Copyright Takatoshi Kondo 2024
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "../common/test_main.hpp"
#include "../common/global_fixture.hpp"

#include <async_mqtt/protocol/connection.hpp>

BOOST_AUTO_TEST_SUITE(ut_connection)

namespace am = async_mqtt;


BOOST_AUTO_TEST_CASE(v5_connect_connack) {
    am::connection<am::role::client> c{am::protocol_version::v5};

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
            [&](am::event_send const& ev) {
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
            [&](am::event_timer const& ev) {
                BOOST_TEST(ev.get_op() == am::event_timer::op_type::reset);
                BOOST_TEST(ev.get_timer_for() == am::timer::pingreq_send);
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

    events = c.recv(recv_connack, recv_connack + sizeof(recv_connack));
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
                [&](am::event_packet_received const& ev) {
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

BOOST_AUTO_TEST_SUITE_END()
