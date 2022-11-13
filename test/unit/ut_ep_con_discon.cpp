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

BOOST_AUTO_TEST_SUITE(ut_ep_con_discon)

namespace am = async_mqtt;
namespace as = boost::asio;

BOOST_AUTO_TEST_CASE(valid_v3_1_1) {
    auto version = am::protocol_version::v3_1_1;
    as::io_context ioc;
    auto guard = as::make_work_guard(ioc.get_executor());
    std::thread th {
        [&] {
            ioc.run();
        }
    };

    auto connect = am::v3_1_1::connect_packet{
        true,   // clean_session
        0x1234, // keep_alive
        am::allocate_buffer("cid1"),
        am::nullopt, // will
        am::allocate_buffer("user1"),
        am::allocate_buffer("pass1")
    };

    auto connack = am::v3_1_1::connack_packet{
        true,   // session_present
        am::connect_return_code::accepted
    };

    auto disconnect = am::v3_1_1::disconnect_packet{};

    auto publish = am::v3_1_1::publish_packet(
        0x1234, // packet_id
        am::allocate_buffer("topic1"),
        am::allocate_buffer("payload1"),
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes
    );

    auto puback = am::v3_1_1::puback_packet(
        0x1234 // packet_id
    );

    auto pubrec = am::v3_1_1::pubrec_packet(
        0x1234 // packet_id
    );

    auto pubrel = am::v3_1_1::pubrel_packet(
        0x1234 // packet_id
    );

    auto pubcomp = am::v3_1_1::pubcomp_packet(
        0x1234 // packet_id
    );

    auto subscribe = am::v3_1_1::subscribe_packet{
        0x1234,         // packet_id
        std::vector<am::topic_subopts> {
            {am::allocate_buffer("topic1"), am::qos::at_most_once},
            {am::allocate_buffer("topic2"), am::qos::exactly_once},
        }
    };

    auto unsubscribe = am::v3_1_1::unsubscribe_packet{
        0x1234,         // packet_id
        std::vector<am::buffer> {
            am::allocate_buffer("topic1"),
            am::allocate_buffer("topic2"),
        }
    };

    auto pingreq = am::v3_1_1::pingreq_packet();

    auto close = am::make_error(am::errc::network_reset, "pseudo close");

    am::endpoint<async_mqtt::role::client, async_mqtt::stub_socket> ep{
        version,
        // for stub_socket args
        version,
        ioc,
        std::deque<am::packet_variant> {
            // receive packets
            connack,
            close,
            connack,
            close,
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

    // send valid packets
    ep.stream().next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(publish, wp));
        }
    );
    {
        auto ec = ep.send(publish, as::use_future).get();
        BOOST_TEST(!ec);
    }

    ep.stream().next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(puback, wp));
        }
    );
    {
        auto ec = ep.send(puback, as::use_future).get();
        BOOST_TEST(!ec);
    }

    ep.stream().next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(pubrec, wp));
        }
    );
    {
        auto ec = ep.send(pubrec, as::use_future).get();
        BOOST_TEST(!ec);
    }

    ep.stream().next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(pubrel, wp));
        }
    );
    {
        auto ec = ep.send(pubrel, as::use_future).get();
        BOOST_TEST(!ec);
    }

    ep.stream().next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(pubcomp, wp));
        }
    );
    {
        auto ec = ep.send(pubcomp, as::use_future).get();
        BOOST_TEST(!ec);
    }

    ep.stream().next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(subscribe, wp));
        }
    );
    {
        auto ec = ep.send(subscribe, as::use_future).get();
        BOOST_TEST(!ec);
    }

    ep.stream().next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(unsubscribe, wp));
        }
    );
    {
        auto ec = ep.send(unsubscribe, as::use_future).get();
        BOOST_TEST(!ec);
    }

    ep.stream().next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(pingreq, wp));
        }
    );
    {
        auto ec = ep.send(pingreq, as::use_future).get();
        BOOST_TEST(!ec);
    }


    // send disconnect
    ep.stream().next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(disconnect, wp));
        }
    );
    {
        auto ec = ep.send(disconnect, as::use_future).get();
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

    // recv connack
    {
        auto pv = ep.recv(as::use_future).get();
        BOOST_TEST(am::packet_compare(connack, pv));
    }

    // send disconnect
    ep.stream().next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(disconnect, wp));
        }
    );
    {
        auto ec = ep.send(disconnect, as::use_future).get();
        BOOST_TEST(!ec);
    }

    // recv close
    {
        auto pv = ep.recv(as::use_future).get();
        BOOST_TEST(pv.get_if<am::system_error>() != nullptr);
    }

    guard.reset();
    th.join();
}

BOOST_AUTO_TEST_CASE(invalid_v3_1_1) {
    auto version = am::protocol_version::v3_1_1;
    as::io_context ioc;
    auto guard = as::make_work_guard(ioc.get_executor());
    std::thread th {
        [&] {
            ioc.run();
        }
    };

    auto connect = am::v3_1_1::connect_packet{
        true,   // clean_session
        0x1234, // keep_alive
        am::allocate_buffer("cid1"),
        am::nullopt, // will
        am::allocate_buffer("user1"),
        am::allocate_buffer("pass1")
    };

    auto connack = am::v3_1_1::connack_packet{
        false,   // session_present
        am::connect_return_code::not_authorized
    };

    auto disconnect = am::v3_1_1::disconnect_packet{};

    auto publish = am::v3_1_1::publish_packet(
        0x1234, // packet_id
        am::allocate_buffer("topic1"),
        am::allocate_buffer("payload1"),
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes
    );

    auto puback = am::v3_1_1::puback_packet(
        0x1234 // packet_id
    );

    auto pubrec = am::v3_1_1::pubrec_packet(
        0x1234 // packet_id
    );

    auto pubrel = am::v3_1_1::pubrel_packet(
        0x1234 // packet_id
    );

    auto pubcomp = am::v3_1_1::pubcomp_packet(
        0x1234 // packet_id
    );

    auto subscribe = am::v3_1_1::subscribe_packet{
        0x1234,         // packet_id
        std::vector<am::topic_subopts> {
            {am::allocate_buffer("topic1"), am::qos::at_most_once},
            {am::allocate_buffer("topic2"), am::qos::exactly_once},
        }
    };

    auto suback = am::v3_1_1::suback_packet{
        0x1234,         // packet_id
        std::vector<am::suback_return_code> {
            am::suback_return_code::success_maximum_qos_1,
            am::suback_return_code::failure
        }
    };

    auto unsubscribe = am::v3_1_1::unsubscribe_packet{
        0x1234,         // packet_id
        std::vector<am::buffer> {
            am::allocate_buffer("topic1"),
            am::allocate_buffer("topic2"),
        }
    };

    auto unsuback = am::v3_1_1::unsuback_packet(
        0x1234 // packet_id
    );

    auto pingreq = am::v3_1_1::pingreq_packet();
    auto pingresp = am::v3_1_1::pingresp_packet();

    auto close = am::make_error(am::errc::network_reset, "pseudo close");

    am::endpoint<async_mqtt::role::client, async_mqtt::stub_socket> ep{
        version,
        // for stub_socket args
        version,
        ioc,
        std::deque<am::packet_variant> {
            // receive packets
            connack
        }
    };

    // send protocol error packets

    // compile error as expected
#if defined(ASYNC_MQTT_TEST_COMPILE_ERROR)

    ep.stream().next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(connack, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.stream().next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(suback, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.stream().next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(unsuback, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.stream().next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(pingresp, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

#endif // defined(ASYNC_MQTT_TEST_COMPILE_ERROR)

    // runtime error due to connot send packet
    ep.stream().next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(am::packet_variant(connack), as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.stream().next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(am::packet_variant(suback), as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.stream().next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(am::packet_variant(unsuback), as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.stream().next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(am::packet_variant(pingresp), as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    // runtime error due to send before sending connect
    ep.stream().next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(publish, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.stream().next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(puback, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.stream().next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(pubrec, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.stream().next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(pubrel, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.stream().next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(pubcomp, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.stream().next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(subscribe, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.stream().next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(unsubscribe, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.stream().next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(pingreq, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
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

    // runtime error due to send before receiving connack
    ep.stream().next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(publish, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.stream().next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(puback, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.stream().next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(pubrec, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.stream().next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(pubrel, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.stream().next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(pubcomp, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.stream().next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(subscribe, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.stream().next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(unsubscribe, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.stream().next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(pingreq, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    // recv connack
    {
        auto pv = ep.recv(as::use_future).get();
        BOOST_TEST(am::packet_compare(connack, pv));
    }

    // runtime error due to send before receiving connack with not_authorized
    ep.stream().next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(publish, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.stream().next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(puback, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.stream().next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(pubrec, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.stream().next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(pubrel, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.stream().next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(pubcomp, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.stream().next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(subscribe, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.stream().next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(unsubscribe, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.stream().next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(pingreq, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    guard.reset();
    th.join();
}

BOOST_AUTO_TEST_SUITE_END()
