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

BOOST_AUTO_TEST_SUITE(ut_ep_con_discon)

namespace am = async_mqtt;
namespace as = boost::asio;

// packet_id is hard coded in this test case for just testing.
// but users need to get packet_id via ep.acquire_unique_packet_id(...)
// see other test cases.

// v3_1_1

BOOST_AUTO_TEST_CASE(valid_client_v3_1_1) {
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

    auto connect = am::v3_1_1::connect_packet{
        true,   // clean_session
        0x1234, // keep_alive
        "cid1",
        std::nullopt, // will
        "user1",
        "pass1"
    };

    auto connack = am::v3_1_1::connack_packet{
        true,   // session_present
        am::connect_return_code::accepted
    };

    auto disconnect = am::v3_1_1::disconnect_packet{};

    auto publish = am::v3_1_1::publish_packet(
        0x1, // hard coded packet_id for just testing
        "topic1",
        "payload1",
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes
    );

    auto puback = am::v3_1_1::puback_packet(
        0x1 // hard coded packet_id for just testing
    );

    auto pubrec = am::v3_1_1::pubrec_packet(
        0x1 // hard coded packet_id for just testing
    );

    auto pubrel = am::v3_1_1::pubrel_packet(
        0x1 // hard coded packet_id for just testing
    );

    auto pubcomp = am::v3_1_1::pubcomp_packet(
        0x1 // hard coded packet_id for just testing
    );

    auto subscribe = am::v3_1_1::subscribe_packet{
        0x1, // hard coded packet_id for just testing
        std::vector<am::topic_subopts> {
            {"topic1", am::qos::at_most_once},
            {"topic2", am::qos::exactly_once},
        }
    };

    auto unsubscribe = am::v3_1_1::unsubscribe_packet{
        0x1, // hard coded packet_id for just testing
        std::vector<am::topic_sharename> {
            am::topic_sharename{"topic1"},
            am::topic_sharename{"topic2"},
        }
    };

    auto pingreq = am::v3_1_1::pingreq_packet();

    ep.next_layer().set_recv_packets(
        {
            // receive packets
            {connack},
            {am::errc::make_error_code(am::errc::connection_reset)},
            {connack},
            {am::errc::make_error_code(am::errc::connection_reset)},
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

    // recv connack
    {
        auto [ec, pv] = ep.async_recv(as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
        BOOST_TEST(connack == pv);
    }

    // register packet_id for testing
    auto [ec] = ep.async_register_packet_id(0x1, as::as_tuple(as::use_future)).get();
    BOOST_TEST(!ec);
    // send valid packets
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(publish == wp);
        }
    );
    {
        auto [ec] = ep.async_send(publish, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(puback == wp);
        }
    );
    {
        auto [ec] = ep.async_send(puback, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(pubrec == wp);
        }
    );
    {
        auto [ec] = ep.async_send(pubrec, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(pubrel == wp);
        }
    );
    {
        auto [ec] = ep.async_send(pubrel, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(pubcomp == wp);
        }
    );
    {
        auto [ec] = ep.async_send(pubcomp, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(subscribe == wp);
        }
    );
    {
        auto [ec] = ep.async_send(subscribe, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(unsubscribe == wp);
        }
    );
    {
        auto [ec] = ep.async_send(unsubscribe, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(pingreq == wp);
        }
    );
    {
        auto [ec] = ep.async_send(pingreq, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }


    // send disconnect
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(disconnect == wp);
        }
    );
    {
        auto [ec] = ep.async_send(disconnect, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }

    // recv close
    {
        auto [ec, pv] = ep.async_recv(as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::errc::connection_reset);
        BOOST_TEST(!pv);
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
        BOOST_TEST(connack == pv);
    }

    // send disconnect
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(disconnect == wp);
        }
    );
    {
        auto [ec] = ep.async_send(disconnect, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }

    // recv close
    {
        auto [ec, pv] = ep.async_recv(as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::errc::connection_reset);
        BOOST_TEST(!pv);
    }

    guard.reset();
    th.join();
}

BOOST_AUTO_TEST_CASE(invalid_client_v3_1_1) {
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

    auto connect = am::v3_1_1::connect_packet{
        true,   // clean_session
        0x1234, // keep_alive
        "cid1",
        std::nullopt, // will
        "user1",
        "pass1"
    };

    auto connack = am::v3_1_1::connack_packet{
        false,   // session_present
        am::connect_return_code::not_authorized
    };

    auto disconnect = am::v3_1_1::disconnect_packet{};

    auto publish = am::v3_1_1::publish_packet(
        0x1, // hard coded packet_id for just testing
        "topic1",
        "payload1",
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes
    );

    auto puback = am::v3_1_1::puback_packet(
        0x1 // hard coded packet_id for just testing
    );

    auto pubrec = am::v3_1_1::pubrec_packet(
        0x1 // hard coded packet_id for just testing
    );

    auto pubrel = am::v3_1_1::pubrel_packet(
        0x1 // hard coded packet_id for just testing
    );

    auto pubcomp = am::v3_1_1::pubcomp_packet(
        0x1 // hard coded packet_id for just testing
    );

    auto subscribe = am::v3_1_1::subscribe_packet{
        0x1, // hard coded packet_id for just testing
        std::vector<am::topic_subopts> {
            {"topic1", am::qos::at_most_once},
            {"topic2", am::qos::exactly_once},
        }
    };

    auto suback = am::v3_1_1::suback_packet{
        0x1, // hard coded packet_id for just testing
        std::vector<am::suback_return_code> {
            am::suback_return_code::success_maximum_qos_1,
            am::suback_return_code::failure
        }
    };

    auto unsubscribe = am::v3_1_1::unsubscribe_packet{
        0x1, // hard coded packet_id for just testing
        std::vector<am::topic_sharename> {
            am::topic_sharename{"topic1"},
            am::topic_sharename{"topic2"},
        }
    };

    auto unsuback = am::v3_1_1::unsuback_packet(
        0x1 // hard coded packet_id for just testing
    );

    auto pingreq = am::v3_1_1::pingreq_packet();
    auto pingresp = am::v3_1_1::pingresp_packet();

    ep.next_layer().set_recv_packets(
        {
            // receive packets
            {connack}
        }
    );

    // send protocol error packets

    // compile error as expected
#if defined(ASYNC_MQTT_TEST_COMPILE_ERROR)

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(connack, as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(suback, as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(unsuback, as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(pingresp, as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

#endif // defined(ASYNC_MQTT_TEST_COMPILE_ERROR)

    // runtime error due to unable send packet
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(am::packet_variant(connack), as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(am::packet_variant(suback), as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(am::packet_variant(unsuback), as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(am::packet_variant(pingresp), as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

    // register packet_id for testing
    auto [ec] = ep.async_register_packet_id(0x1, as::as_tuple(as::use_future)).get();
    BOOST_TEST(!ec);
    // offline publish success
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(publish, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }

    // runtime error due to send before sending connect
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(puback, as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(pubrec, as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(pubrel, as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(pubcomp, as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(subscribe, as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(unsubscribe, as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(pingreq, as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
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

    // register packet_id for testing
    {
        auto [ec] = ep.async_register_packet_id(0x1, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
        // offline publish success
        ep.next_layer().set_write_packet_checker(
            [&](am::packet_variant) {
                BOOST_TEST(false);
            }
        );
    }
    {
        auto [ec] = ep.async_send(publish, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }

    // runtime error due to send before receiving connack
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(puback, as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(pubrec, as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(pubrel, as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(pubcomp, as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(subscribe, as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(unsubscribe, as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(pingreq, as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

    // recv connack
    {
        auto [ec, pv] = ep.async_recv(as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
        BOOST_TEST(connack == pv);
    }

    // offline publish success
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(publish, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }

    // runtime error due to send before receiving connack with not_authorized
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(puback, as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(pubrec, as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(pubrel, as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(pubcomp, as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(subscribe, as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(unsubscribe, as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(pingreq, as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }
    ep.async_close(as::as_tuple(as::use_future)).get();
    guard.reset();
    th.join();
}

BOOST_AUTO_TEST_CASE(valid_server_v3_1_1) {
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

    auto connect = am::v3_1_1::connect_packet{
        true,   // clean_session
        0x1234, // keep_alive
        "cid1",
        std::nullopt, // will
        "user1",
        "pass1"
    };

    auto connack = am::v3_1_1::connack_packet{
        true,   // session_present
        am::connect_return_code::accepted
    };

    auto publish = am::v3_1_1::publish_packet(
        0x1, // hard coded packet_id for just testing
        "topic1",
        "payload1",
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes
    );

    auto puback = am::v3_1_1::puback_packet(
        0x1 // hard coded packet_id for just testing
    );

    auto pubrec = am::v3_1_1::pubrec_packet(
        0x1 // hard coded packet_id for just testing
    );

    auto pubrel = am::v3_1_1::pubrel_packet(
        0x1 // hard coded packet_id for just testing
    );

    auto pubcomp = am::v3_1_1::pubcomp_packet(
        0x1 // hard coded packet_id for just testing
    );

    auto suback = am::v3_1_1::suback_packet{
        0x1, // hard coded packet_id for just testing
        std::vector<am::suback_return_code> {
            am::suback_return_code::success_maximum_qos_1,
            am::suback_return_code::failure
        }
    };

    auto unsuback = am::v3_1_1::unsuback_packet(
        0x1 // hard coded packet_id for just testing
    );

    auto pingresp = am::v3_1_1::pingresp_packet();

    ep1.next_layer().set_recv_packets(
        {
            // receive packets
            {connect},
            {am::errc::make_error_code(am::errc::connection_reset)},
        }
    );

    // recv connect
    {
        auto [ec, pv] = ep1.async_recv(as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
        BOOST_TEST(connect == pv);
    }

    // send connack
    ep1.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(connack == wp);
        }
    );
    {
        auto [ec] = ep1.async_send(connack, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }

    // register packet_id for testing
    auto [ec1] = ep1.async_register_packet_id(0x1, as::as_tuple(as::use_future)).get();
    BOOST_TEST(!ec1);

    // send valid packets
    ep1.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(publish == wp);
        }
    );
    {
        auto [ec] = ep1.async_send(publish, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }

    ep1.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(puback == wp);
        }
    );
    {
        auto [ec] = ep1.async_send(puback, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }

    ep1.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(pubrec == wp);
        }
    );
    {
        auto [ec] = ep1.async_send(pubrec, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }

    ep1.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(pubrel == wp);
        }
    );
    {
        auto [ec] = ep1.async_send(pubrel, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }

    ep1.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(pubcomp == wp);
        }
    );
    {
        auto [ec] = ep1.async_send(pubcomp, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }

    ep1.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(suback == wp);
        }
    );
    {
        auto [ec] = ep1.async_send(suback, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }

    ep1.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(unsuback == wp);
        }
    );
    {
        auto [ec] = ep1.async_send(unsuback, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }

    ep1.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(pingresp == wp);
        }
    );
    {
        auto [ec] = ep1.async_send(pingresp, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }


    // recv close
    {
        auto [ec, pv] = ep1.async_recv(as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::errc::connection_reset);
        BOOST_TEST(!pv);
    }

    ep2.next_layer().set_recv_packets(
        {
            // receive packets
            {connect},
            {am::errc::make_error_code(am::errc::connection_reset)},
        }
    );

    // recv connect
    {
        auto [ec, pv] = ep2.async_recv(as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
        BOOST_TEST(connect == pv);
    }

    // send connack
    ep2.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(connack == wp);
        }
    );
    {
        auto [ec] = ep2.async_send(connack, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }

    // register packet_id for testing
    auto [ec2] = ep2.async_register_packet_id(0x1, as::as_tuple(as::use_future)).get();
    BOOST_TEST(!ec2);

    // send publish behalf of valid packets
    ep2.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(publish == wp);
        }
    );
    {
        auto [ec] = ep2.async_send(publish, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }

    // recv close
    {
        auto [ec, pv] = ep2.async_recv(as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::errc::connection_reset);
        BOOST_TEST(!pv);
    }

    guard.reset();
    th.join();
}

BOOST_AUTO_TEST_CASE(invalid_server_v3_1_1) {
    auto version = am::protocol_version::v3_1_1;
    as::io_context ioc;
    auto guard = as::make_work_guard(ioc.get_executor());
    std::thread th {
        [&] {
            ioc.run();
        }
    };

    auto ep = am::endpoint<async_mqtt::role::server, async_mqtt::stub_socket>{
        version,
        // for stub_socket args
        version,
        ioc.get_executor()
    };

    auto connect = am::v3_1_1::connect_packet{
        true,   // clean_session
        0x1234, // keep_alive
        "cid1",
        std::nullopt, // will
        "user1",
        "pass1"
    };

    auto connack = am::v3_1_1::connack_packet{
        false,   // session_present
        am::connect_return_code::not_authorized
    };

    auto disconnect = am::v3_1_1::disconnect_packet{};

    auto publish = am::v3_1_1::publish_packet(
        0x1, // hard coded packet_id for just testing
        "topic1",
        "payload1",
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes
    );

    auto puback = am::v3_1_1::puback_packet(
        0x1 // hard coded packet_id for just testing
    );

    auto pubrec = am::v3_1_1::pubrec_packet(
        0x1 // hard coded packet_id for just testing
    );

    auto pubrel = am::v3_1_1::pubrel_packet(
        0x1 // hard coded packet_id for just testing
    );

    auto pubcomp = am::v3_1_1::pubcomp_packet(
        0x1 // hard coded packet_id for just testing
    );

    auto subscribe = am::v3_1_1::subscribe_packet{
        0x1, // hard coded packet_id for just testing
        std::vector<am::topic_subopts> {
            {"topic1", am::qos::at_most_once},
            {"topic2", am::qos::exactly_once},
        }
    };

    auto suback = am::v3_1_1::suback_packet{
        0x1, // hard coded packet_id for just testing
        std::vector<am::suback_return_code> {
            am::suback_return_code::success_maximum_qos_1,
            am::suback_return_code::failure
        }
    };

    auto unsubscribe = am::v3_1_1::unsubscribe_packet{
        0x1, // hard coded packet_id for just testing
        std::vector<am::topic_sharename> {
            am::topic_sharename{"topic1"},
            am::topic_sharename{"topic2"},
        }
    };

    auto unsuback = am::v3_1_1::unsuback_packet(
        0x1 // hard coded packet_id for just testing
    );

    auto pingreq = am::v3_1_1::pingreq_packet();
    auto pingresp = am::v3_1_1::pingresp_packet();

    ep.next_layer().set_recv_packets(
        {
            // receive packets
            {connect},
        }
    );

    // send protocol error packets

    // compile error as expected
#if defined(ASYNC_MQTT_TEST_COMPILE_ERROR)

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(connect, as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(subscribe, as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(unsubscribe, as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(pingreq, as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

#endif // defined(ASYNC_MQTT_TEST_COMPILE_ERROR)

    // runtime error due to unable send packet
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(am::packet_variant(connect), as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(am::packet_variant(subscribe), as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(am::packet_variant(unsubscribe), as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(am::packet_variant(pingreq), as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

    // register packet_id for testing
    auto [ec] = ep.async_register_packet_id(0x1, as::as_tuple(as::use_future)).get();
    BOOST_TEST(!ec);
    // offline publish success
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(publish, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }

    // runtime error due to send before receiving connect
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(puback, as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(pubrec, as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(pubrel, as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(pubcomp, as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(suback, as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(unsuback, as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(pingresp, as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

    // recv connect
    {
        auto [ec, pv] = ep.async_recv(as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
        BOOST_TEST(connect == pv);
    }

    // offline publish success
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(publish, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }

    // runtime error due to send before sending connack
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(puback, as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(pubrec, as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(pubrel, as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(pubcomp, as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(suback, as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(unsuback, as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(pingresp, as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
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

    // offline publish success
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(publish, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }

    // runtime error due to send before receiving connack with not_authorized
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(puback, as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(pubrec, as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(pubrel, as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(pubcomp, as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(suback, as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(unsuback, as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(pingresp, as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }
    ep.async_close(as::as_tuple(as::use_future)).get();
    guard.reset();
    th.join();
}

// v5

BOOST_AUTO_TEST_CASE(valid_client_v5) {
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
        am::properties{}
    };

    auto disconnect = am::v5::disconnect_packet{
        am::disconnect_reason_code::normal_disconnection,
        am::properties{}
    };

    auto publish = am::v5::publish_packet(
        0x1, // hard coded packet_id for just testing
        "topic1",
        "payload1",
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{}
    );

    auto puback = am::v5::puback_packet(
        0x1, // hard coded packet_id for just testing
        am::puback_reason_code::success,
        am::properties{}
    );

    auto pubrec = am::v5::pubrec_packet(
        0x1, // hard coded packet_id for just testing
        am::pubrec_reason_code::success,
        am::properties{}
    );

    auto pubrel = am::v5::pubrel_packet(
        0x1, // hard coded packet_id for just testing
        am::pubrel_reason_code::success,
        am::properties{}
    );

    auto pubcomp = am::v5::pubcomp_packet(
        0x1, // hard coded packet_id for just testing
        am::pubcomp_reason_code::success,
        am::properties{}
    );

    auto subscribe = am::v5::subscribe_packet{
        0x1, // hard coded packet_id for just testing
        std::vector<am::topic_subopts> {
            {"topic1", am::qos::at_most_once},
            {"topic2", am::qos::exactly_once},
        },
        am::properties{}
    };

    auto unsubscribe = am::v5::unsubscribe_packet{
        0x1, // hard coded packet_id for just testing
        std::vector<am::topic_sharename> {
            am::topic_sharename{"topic1"},
            am::topic_sharename{"topic2"},
        },
        am::properties{}
    };

    auto pingreq = am::v5::pingreq_packet();

    auto auth = am::v5::auth_packet{
        am::auth_reason_code::continue_authentication,
        am::properties{}
    };

    ep.next_layer().set_recv_packets(
        {
            // receive packets
            {connack},
            {am::errc::make_error_code(am::errc::connection_reset)},
            {connack},
            {am::errc::make_error_code(am::errc::connection_reset)},
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

    // send auth packet that can be sent before connack receiving
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(auth == wp);
        }
    );
    {
        auto [ec] = ep.async_send(auth, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }

    // recv connack
    {
        auto [ec, pv] = ep.async_recv(as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
        BOOST_TEST(connack == pv);
    }

    // send valid packets
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(auth == wp);
        }
    );
    {
        auto [ec] = ep.async_send(auth, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }

    // register packet_id for testing
    auto [ec] = ep.async_register_packet_id(0x1, as::as_tuple(as::use_future)).get();
    BOOST_TEST(!ec);
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(publish == wp);
        }
    );
    {
        auto [ec] = ep.async_send(publish, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(puback == wp);
        }
    );
    {
        auto [ec] = ep.async_send(puback, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(pubrec == wp);
        }
    );
    {
        auto [ec] = ep.async_send(pubrec, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(pubrel == wp);
        }
    );
    {
        auto [ec] = ep.async_send(pubrel, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(pubcomp == wp);
        }
    );
    {
        auto [ec] = ep.async_send(pubcomp, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(subscribe == wp);
        }
    );
    {
        auto [ec] = ep.async_send(subscribe, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(unsubscribe == wp);
        }
    );
    {
        auto [ec] = ep.async_send(unsubscribe, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(pingreq == wp);
        }
    );
    {
        auto [ec] = ep.async_send(pingreq, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }


    // send disconnect
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(disconnect == wp);
        }
    );
    {
        auto [ec] = ep.async_send(disconnect, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }

    // recv close
    {
        auto [ec, pv] = ep.async_recv(as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::errc::connection_reset);
        BOOST_TEST(!pv);
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
        BOOST_TEST(connack == pv);
    }

    // send disconnect
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(disconnect == wp);
        }
    );
    {
        auto [ec] = ep.async_send(disconnect, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }

    // recv close
    {
        auto [ec, pv] = ep.async_recv(as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::errc::connection_reset);
        BOOST_TEST(!pv);
    }

    guard.reset();
    th.join();
}

BOOST_AUTO_TEST_CASE(invalid_client_v5) {
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
        true,   // clean_start
        0x1234, // keep_alive
        "cid1",
        std::nullopt, // will
        "user1",
        "pass1",
        am::properties{}
    };

    auto connack = am::v5::connack_packet{
        false,   // session_present
        am::connect_reason_code::not_authorized,
        am::properties{}
    };

    auto disconnect = am::v5::disconnect_packet{
        am::disconnect_reason_code::normal_disconnection,
        am::properties{}
    };

    auto publish = am::v5::publish_packet(
        0x1, // hard coded packet_id for just testing
        "topic1",
        "payload1",
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{}
    );

    auto puback = am::v5::puback_packet(
        0x1, // hard coded packet_id for just testing
        am::puback_reason_code::success,
        am::properties{}
    );

    auto pubrec = am::v5::pubrec_packet(
        0x1, // hard coded packet_id for just testing
        am::pubrec_reason_code::success,
        am::properties{}
    );

    auto pubrel = am::v5::pubrel_packet(
        0x1, // hard coded packet_id for just testing
        am::pubrel_reason_code::success,
        am::properties{}
    );

    auto pubcomp = am::v5::pubcomp_packet(
        0x1, // hard coded packet_id for just testing
        am::pubcomp_reason_code::success,
        am::properties{}
    );

    auto subscribe = am::v5::subscribe_packet{
        0x1, // hard coded packet_id for just testing
        std::vector<am::topic_subopts> {
            {"topic1", am::qos::at_most_once},
            {"topic2", am::qos::exactly_once},
        },
        am::properties{}
    };

    auto suback = am::v5::suback_packet{
        0x1, // hard coded packet_id for just testing
        std::vector<am::suback_reason_code> {
            am::suback_reason_code::granted_qos_1,
            am::suback_reason_code::unspecified_error
        },
        am::properties{}
    };

    auto unsubscribe = am::v5::unsubscribe_packet{
        0x1, // hard coded packet_id for just testing
        std::vector<am::topic_sharename> {
            am::topic_sharename{"topic1"},
            am::topic_sharename{"topic2"},
        },
        am::properties{}
    };


    auto unsuback = am::v5::unsuback_packet(
        0x1, // hard coded packet_id for just testing
        std::vector<am::unsuback_reason_code> {
            am::unsuback_reason_code::no_subscription_existed,
            am::unsuback_reason_code::unspecified_error
        },
        am::properties{}
    );

    auto pingreq = am::v5::pingreq_packet();
    auto pingresp = am::v5::pingresp_packet();

    auto auth = am::v5::auth_packet{
        am::auth_reason_code::continue_authentication,
        am::properties{}
    };

    ep.next_layer().set_recv_packets(
        {
            // receive packets
            {connack},
        }
    );

    // send protocol error packets

    // compile error as expected
#if defined(ASYNC_MQTT_TEST_COMPILE_ERROR)

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(connack, as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(suback, as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(unsuback, as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(pingresp, as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

#endif // defined(ASYNC_MQTT_TEST_COMPILE_ERROR)

    // runtime error due to unable send packet
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(am::packet_variant(connack), as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(am::packet_variant(suback), as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(am::packet_variant(unsuback), as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(am::packet_variant(pingresp), as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

    // register packet_id for testing
    auto [ec] = ep.async_register_packet_id(0x1, as::as_tuple(as::use_future)).get();
    BOOST_TEST(!ec);
    // offline publish success
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(publish, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }

    // runtime error due to send before sending connect
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(puback, as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(pubrec, as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(pubrel, as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(pubcomp, as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(subscribe, as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(unsubscribe, as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(pingreq, as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(auth, as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
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

    // register packet_id for testing
    {
        auto [ec] = ep.async_register_packet_id(0x1, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
        // offline publish success
        ep.next_layer().set_write_packet_checker(
            [&](am::packet_variant) {
                BOOST_TEST(false);
            }
        );
    }
    {
        auto [ec] = ep.async_send(publish, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }

    // runtime error due to send before receiving connack
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(puback, as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(pubrec, as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(pubrel, as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(pubcomp, as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(subscribe, as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(unsubscribe, as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(pingreq, as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

    // recv connack
    {
        auto [ec, pv] = ep.async_recv(as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
        BOOST_TEST(connack == pv);
    }

    // offline publish success
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(publish, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }

    // runtime error due to send before receiving connack with not_authorized
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(puback, as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(pubrec, as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(pubrel, as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(pubcomp, as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(subscribe, as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(unsubscribe, as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(pingreq, as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }
    ep.async_close(as::as_tuple(as::use_future)).get();
    guard.reset();
    th.join();
}

BOOST_AUTO_TEST_CASE(valid_server_v5) {
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
        am::properties{}
    };

    auto disconnect = am::v5::disconnect_packet{
        am::disconnect_reason_code::normal_disconnection,
        am::properties{}
    };

    auto publish = am::v5::publish_packet(
        0x1, // hard coded packet_id for just testing
        "topic1",
        "payload1",
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{}
    );

    auto puback = am::v5::puback_packet(
        0x1, // hard coded packet_id for just testing
        am::puback_reason_code::success,
        am::properties{}
    );

    auto pubrec = am::v5::pubrec_packet(
        0x1, // hard coded packet_id for just testing
        am::pubrec_reason_code::success,
        am::properties{}
    );

    auto pubrel = am::v5::pubrel_packet(
        0x1, // hard coded packet_id for just testing
        am::pubrel_reason_code::success,
        am::properties{}
    );

    auto pubcomp = am::v5::pubcomp_packet(
        0x1, // hard coded packet_id for just testing
        am::pubcomp_reason_code::success,
        am::properties{}
    );

    auto suback = am::v5::suback_packet{
        0x1, // hard coded packet_id for just testing
        std::vector<am::suback_reason_code> {
            am::suback_reason_code::granted_qos_1,
            am::suback_reason_code::unspecified_error
        },
        am::properties{}
    };

    auto unsuback = am::v5::unsuback_packet(
        0x1, // hard coded packet_id for just testing
        std::vector<am::unsuback_reason_code> {
            am::unsuback_reason_code::no_subscription_existed,
            am::unsuback_reason_code::unspecified_error
        },
        am::properties{}
    );

    auto pingresp = am::v5::pingresp_packet();

    auto auth = am::v5::auth_packet{
        am::auth_reason_code::continue_authentication,
        am::properties{}
    };

    ep1.next_layer().set_recv_packets(
        {
            // receive packets
            {connect},
            {am::errc::make_error_code(am::errc::connection_reset)},
        }
    );

    // recv connect
    {
        auto [ec, pv] = ep1.async_recv(as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
        BOOST_TEST(connect == pv);
    }

    // send auth
    ep1.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(auth == wp);
        }
    );
    {
        auto [ec] = ep1.async_send(auth, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }

    // send connack
    ep1.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(connack == wp);
        }
    );
    {
        auto [ec] = ep1.async_send(connack, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }

    // register packet_id for testing
    auto [ec1] = ep1.async_register_packet_id(0x1, as::as_tuple(as::use_future)).get();
    BOOST_TEST(!ec1);

    // send valid packets
    ep1.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(auth == wp);
        }
    );
    {
        auto [ec] = ep1.async_send(auth, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }

    ep1.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(publish == wp);
        }
    );
    {
        auto [ec] = ep1.async_send(publish, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }

    ep1.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(puback == wp);
        }
    );
    {
        auto [ec] = ep1.async_send(puback, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }

    ep1.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(pubrec == wp);
        }
    );
    {
        auto [ec] = ep1.async_send(pubrec, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }

    ep1.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(pubrel == wp);
        }
    );
    {
        auto [ec] = ep1.async_send(pubrel, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }

    ep1.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(pubcomp == wp);
        }
    );
    {
        auto [ec] = ep1.async_send(pubcomp, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }

    ep1.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(suback == wp);
        }
    );
    {
        auto [ec] = ep1.async_send(suback, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }

    ep1.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(unsuback == wp);
        }
    );
    {
        auto [ec] = ep1.async_send(unsuback, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }

    ep1.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(pingresp == wp);
        }
    );
    {
        auto [ec] = ep1.async_send(pingresp, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }


    // recv close
    {
        auto [ec, pv] = ep1.async_recv(as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::errc::connection_reset);
        BOOST_TEST(!pv);
    }

    ep2.next_layer().set_recv_packets(
        {
            // receive packets
            {connect},
            {am::errc::make_error_code(am::errc::connection_reset)},
        }
    );

    // recv connect
    {
        auto [ec, pv] = ep2.async_recv(as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
        BOOST_TEST(connect == pv);
    }

    // send connack
    ep2.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(connack == wp);
        }
    );
    {
        auto [ec] = ep2.async_send(connack, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }

    // register packet_id for testing
    auto [ec2] = ep2.async_register_packet_id(0x1, as::as_tuple(as::use_future)).get();
    BOOST_TEST(!ec2);

    // send publish behalf of valid packets
    ep2.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(publish == wp);
        }
    );
    {
        auto [ec] = ep2.async_send(publish, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }

    // recv close
    {
        auto [ec, pv] = ep2.async_recv(as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::errc::connection_reset);
        BOOST_TEST(!pv);
    }

    guard.reset();
    th.join();
}

BOOST_AUTO_TEST_CASE(invalid_server_v5) {
    auto version = am::protocol_version::v5;
    as::io_context ioc;
    auto guard = as::make_work_guard(ioc.get_executor());
    std::thread th {
        [&] {
            ioc.run();
        }
    };

    auto ep = am::endpoint<async_mqtt::role::server, async_mqtt::stub_socket>{
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
        false,   // session_present
        am::connect_reason_code::not_authorized,
        am::properties{}
    };

    auto disconnect = am::v5::disconnect_packet{
        am::disconnect_reason_code::normal_disconnection,
        am::properties{}
    };

    auto publish = am::v5::publish_packet(
        0x1, // hard coded packet_id for just testing
        "topic1",
        "payload1",
        am::qos::exactly_once | am::pub::retain::yes | am::pub::dup::yes,
        am::properties{}
    );

    auto puback = am::v5::puback_packet(
        0x1, // hard coded packet_id for just testing
        am::puback_reason_code::success,
        am::properties{}
    );

    auto pubrec = am::v5::pubrec_packet(
        0x1, // hard coded packet_id for just testing
        am::pubrec_reason_code::success,
        am::properties{}
    );

    auto pubrel = am::v5::pubrel_packet(
        0x1, // hard coded packet_id for just testing
        am::pubrel_reason_code::success,
        am::properties{}
    );

    auto pubcomp = am::v5::pubcomp_packet(
        0x1, // hard coded packet_id for just testing
        am::pubcomp_reason_code::success,
        am::properties{}
    );

    auto subscribe = am::v5::subscribe_packet{
        0x1, // hard coded packet_id for just testing
        std::vector<am::topic_subopts> {
            {"topic1", am::qos::at_most_once},
            {"topic2", am::qos::exactly_once},
        },
        am::properties{}
    };

    auto suback = am::v5::suback_packet{
        0x1, // hard coded packet_id for just testing
        std::vector<am::suback_reason_code> {
            am::suback_reason_code::granted_qos_1,
            am::suback_reason_code::unspecified_error
        },
        am::properties{}
    };

    auto unsubscribe = am::v5::unsubscribe_packet{
        0x1, // hard coded packet_id for just testing
        std::vector<am::topic_sharename> {
            am::topic_sharename{"topic1"},
            am::topic_sharename{"topic2"},
        },
        am::properties{}
    };


    auto unsuback = am::v5::unsuback_packet(
        0x1, // hard coded packet_id for just testing
        std::vector<am::unsuback_reason_code> {
            am::unsuback_reason_code::no_subscription_existed,
            am::unsuback_reason_code::unspecified_error
        },
        am::properties{}
    );

    auto pingreq = am::v5::pingreq_packet();
    auto pingresp = am::v5::pingresp_packet();

    auto auth = am::v5::auth_packet{
        am::auth_reason_code::continue_authentication,
        am::properties{}
    };

    auto close = am::packet_variant{};

    ep.next_layer().set_recv_packets(
        {
            // receive packets
            {connect},
        }
    );

    // send protocol error packets

    // compile error as expected
#if defined(ASYNC_MQTT_TEST_COMPILE_ERROR)

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(connect, as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(subscribe, as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(unsubscribe, as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(pingreq, as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

#endif // defined(ASYNC_MQTT_TEST_COMPILE_ERROR)

    // runtime error due to unable send packet
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(am::packet_variant(connect), as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(am::packet_variant(subscribe), as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(am::packet_variant(unsubscribe), as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(am::packet_variant(pingreq), as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

    // register packet_id for testing
    auto [ec] = ep.async_register_packet_id(0x1, as::as_tuple(as::use_future)).get();
    BOOST_TEST(!ec);
    // offline publish success
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(publish, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }

    // runtime error due to send before receiving connect
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(auth, as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(puback, as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(pubrec, as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(pubrel, as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(pubcomp, as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(suback, as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(unsuback, as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(pingresp, as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

    // recv connect
    {
        auto [ec, pv] = ep.async_recv(as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
        BOOST_TEST(connect == pv);
    }

    // offline publish success
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(publish, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }

    // runtime error due to send before sending connack
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(puback, as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(pubrec, as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(pubrel, as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(pubcomp, as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(suback, as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(unsuback, as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(pingresp, as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
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

    // offline publish success
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(publish, as::as_tuple(as::use_future)).get();
        BOOST_TEST(!ec);
    }

    // runtime error due to send before receiving connack with not_authorized
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(puback, as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(pubrec, as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(pubrel, as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(pubcomp, as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(suback, as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(unsuback, as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto [ec] = ep.async_send(pingresp, as::as_tuple(as::use_future)).get();
        BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
    }
    ep.async_close(as::as_tuple(as::use_future)).get();
    guard.reset();
    th.join();
}

BOOST_AUTO_TEST_SUITE_END()
