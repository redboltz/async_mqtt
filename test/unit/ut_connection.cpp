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


BOOST_AUTO_TEST_CASE(send) {
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

    auto [ec, events] = c.send(p);
    BOOST_TEST(!ec);
    BOOST_TEST(events.size() == 2);
    std::visit(
        am::overload{
            [&](am::event_send const& ev) {
                BOOST_TEST(!ev.release_packet_id_required_if_send_error());
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
}

BOOST_AUTO_TEST_SUITE_END()
