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
#include "packet_compare.hpp"

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

    am::endpoint<async_mqtt::role::client, async_mqtt::stub_socket> ep{
        version,
        // for stub_socket args
        version,
        ioc
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
        0x1, // hard coded packet_id for just testing
        am::allocate_buffer("topic1"),
        am::allocate_buffer("payload1"),
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
            {am::allocate_buffer("topic1"), am::qos::at_most_once},
            {am::allocate_buffer("topic2"), am::qos::exactly_once},
        }
    };

    auto unsubscribe = am::v3_1_1::unsubscribe_packet{
        0x1, // hard coded packet_id for just testing
        std::vector<am::topic_sharename> {
            am::allocate_buffer("topic1"),
            am::allocate_buffer("topic2"),
        }
    };

    auto pingreq = am::v3_1_1::pingreq_packet();

    auto close = am::make_error(am::errc::network_reset, "pseudo close");

    ep.next_layer().set_recv_packets(
        {
            // receive packets
            connack,
            close,
            connack,
            close,
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

    // register packet_id for testing
    BOOST_TEST(ep.register_packet_id(0x1, as::use_future).get());
    // send valid packets
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(publish, wp));
        }
    );
    {
        auto ec = ep.send(publish, as::use_future).get();
        BOOST_TEST(!ec);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(puback, wp));
        }
    );
    {
        auto ec = ep.send(puback, as::use_future).get();
        BOOST_TEST(!ec);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(pubrec, wp));
        }
    );
    {
        auto ec = ep.send(pubrec, as::use_future).get();
        BOOST_TEST(!ec);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(pubrel, wp));
        }
    );
    {
        auto ec = ep.send(pubrel, as::use_future).get();
        BOOST_TEST(!ec);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(pubcomp, wp));
        }
    );
    {
        auto ec = ep.send(pubcomp, as::use_future).get();
        BOOST_TEST(!ec);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(subscribe, wp));
        }
    );
    {
        auto ec = ep.send(subscribe, as::use_future).get();
        BOOST_TEST(!ec);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(unsubscribe, wp));
        }
    );
    {
        auto ec = ep.send(unsubscribe, as::use_future).get();
        BOOST_TEST(!ec);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(pingreq, wp));
        }
    );
    {
        auto ec = ep.send(pingreq, as::use_future).get();
        BOOST_TEST(!ec);
    }


    // send disconnect
    ep.next_layer().set_write_packet_checker(
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

    // send disconnect
    ep.next_layer().set_write_packet_checker(
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

BOOST_AUTO_TEST_CASE(invalid_client_v3_1_1) {
    auto version = am::protocol_version::v3_1_1;
    as::io_context ioc;
    auto guard = as::make_work_guard(ioc.get_executor());
    std::thread th {
        [&] {
            ioc.run();
        }
    };

    am::endpoint<async_mqtt::role::client, async_mqtt::stub_socket> ep{
        version,
        // for stub_socket args
        version,
        ioc
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
        0x1, // hard coded packet_id for just testing
        am::allocate_buffer("topic1"),
        am::allocate_buffer("payload1"),
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
            {am::allocate_buffer("topic1"), am::qos::at_most_once},
            {am::allocate_buffer("topic2"), am::qos::exactly_once},
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
            am::allocate_buffer("topic1"),
            am::allocate_buffer("topic2"),
        }
    };

    auto unsuback = am::v3_1_1::unsuback_packet(
        0x1 // hard coded packet_id for just testing
    );

    auto pingreq = am::v3_1_1::pingreq_packet();
    auto pingresp = am::v3_1_1::pingresp_packet();

    auto close = am::make_error(am::errc::network_reset, "pseudo close");

    ep.next_layer().set_recv_packets(
        {
            // receive packets
            connack
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
        auto ec = ep.send(connack, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(suback, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(unsuback, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(pingresp, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

#endif // defined(ASYNC_MQTT_TEST_COMPILE_ERROR)

    // runtime error due to unable send packet
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(am::packet_variant(connack), as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(am::packet_variant(suback), as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(am::packet_variant(unsuback), as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(am::packet_variant(pingresp), as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    // register packet_id for testing
    BOOST_TEST(ep.register_packet_id(0x1, as::use_future).get());
    // offline publish success
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(publish, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::success);
    }

    // runtime error due to send before sending connect
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(puback, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(pubrec, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(pubrel, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(pubcomp, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(subscribe, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(unsubscribe, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(pingreq, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

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

    // register packet_id for testing
    BOOST_TEST(ep.register_packet_id(0x1, as::use_future).get());
    // offline publish success
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(publish, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::success);
    }

    // runtime error due to send before receiving connack
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(puback, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(pubrec, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(pubrel, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(pubcomp, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(subscribe, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(unsubscribe, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.next_layer().set_write_packet_checker(
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

    // offline publish success
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(publish, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::success);
    }

    // runtime error due to send before receiving connack with not_authorized
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(puback, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(pubrec, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(pubrel, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(pubcomp, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(subscribe, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(unsubscribe, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(pingreq, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }
    ep.close(as::use_future).get();
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

    am::endpoint<async_mqtt::role::server, async_mqtt::stub_socket> ep1{
        version,
        // for stub_socket args
        version,
        ioc
    };
    am::endpoint<async_mqtt::role::server, async_mqtt::stub_socket> ep2{
        version,
        // for stub_socket args
        version,
        ioc
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

    auto publish = am::v3_1_1::publish_packet(
        0x1, // hard coded packet_id for just testing
        am::allocate_buffer("topic1"),
        am::allocate_buffer("payload1"),
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

    auto close = am::make_error(am::errc::network_reset, "pseudo close");

    ep1.next_layer().set_recv_packets(
        {
            // receive packets
            connect,
            close,
        }
    );

    // recv connect
    {
        auto pv = ep1.recv(as::use_future).get();
        BOOST_TEST(am::packet_compare(connect, pv));
    }

    // send connack
    ep1.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(connack, wp));
        }
    );
    {
        auto ec = ep1.send(connack, as::use_future).get();
        BOOST_TEST(!ec);
    }

    // register packet_id for testing
    BOOST_TEST(ep1.register_packet_id(0x1, as::use_future).get());

    // send valid packets
    ep1.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(publish, wp));
        }
    );
    {
        auto ec = ep1.send(publish, as::use_future).get();
        BOOST_TEST(!ec);
    }

    ep1.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(puback, wp));
        }
    );
    {
        auto ec = ep1.send(puback, as::use_future).get();
        BOOST_TEST(!ec);
    }

    ep1.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(pubrec, wp));
        }
    );
    {
        auto ec = ep1.send(pubrec, as::use_future).get();
        BOOST_TEST(!ec);
    }

    ep1.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(pubrel, wp));
        }
    );
    {
        auto ec = ep1.send(pubrel, as::use_future).get();
        BOOST_TEST(!ec);
    }

    ep1.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(pubcomp, wp));
        }
    );
    {
        auto ec = ep1.send(pubcomp, as::use_future).get();
        BOOST_TEST(!ec);
    }

    ep1.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(suback, wp));
        }
    );
    {
        auto ec = ep1.send(suback, as::use_future).get();
        BOOST_TEST(!ec);
    }

    ep1.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(unsuback, wp));
        }
    );
    {
        auto ec = ep1.send(unsuback, as::use_future).get();
        BOOST_TEST(!ec);
    }

    ep1.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(pingresp, wp));
        }
    );
    {
        auto ec = ep1.send(pingresp, as::use_future).get();
        BOOST_TEST(!ec);
    }


    // recv close
    {
        auto pv = ep1.recv(as::use_future).get();
        BOOST_TEST(pv.get_if<am::system_error>() != nullptr);
    }

    ep2.next_layer().set_recv_packets(
        {
            // receive packets
            connect,
            close,
        }
    );

    // recv connect
    {
        auto pv = ep2.recv(as::use_future).get();
        BOOST_TEST(am::packet_compare(connect, pv));
    }

    // send connack
    ep2.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(connack, wp));
        }
    );
    {
        auto ec = ep2.send(connack, as::use_future).get();
        BOOST_TEST(!ec);
    }

    // register packet_id for testing
    BOOST_TEST(ep2.register_packet_id(0x1, as::use_future).get());

    // send publish behalf of valid packets
    ep2.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(publish, wp));
        }
    );
    {
        auto ec = ep2.send(publish, as::use_future).get();
        BOOST_TEST(!ec);
    }

    // recv close
    {
        auto pv = ep2.recv(as::use_future).get();
        BOOST_TEST(pv.get_if<am::system_error>() != nullptr);
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

    am::endpoint<async_mqtt::role::server, async_mqtt::stub_socket> ep{
        version,
        // for stub_socket args
        version,
        ioc
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
        0x1, // hard coded packet_id for just testing
        am::allocate_buffer("topic1"),
        am::allocate_buffer("payload1"),
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
            {am::allocate_buffer("topic1"), am::qos::at_most_once},
            {am::allocate_buffer("topic2"), am::qos::exactly_once},
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
            am::allocate_buffer("topic1"),
            am::allocate_buffer("topic2"),
        }
    };

    auto unsuback = am::v3_1_1::unsuback_packet(
        0x1 // hard coded packet_id for just testing
    );

    auto pingreq = am::v3_1_1::pingreq_packet();
    auto pingresp = am::v3_1_1::pingresp_packet();

    auto close = am::make_error(am::errc::network_reset, "pseudo close");

    ep.next_layer().set_recv_packets(
        {
            // receive packets
            connect,
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
        auto ec = ep.send(connect, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(subscribe, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(unsubscribe, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(pingreq, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

#endif // defined(ASYNC_MQTT_TEST_COMPILE_ERROR)

    // runtime error due to unable send packet
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(am::packet_variant(connect), as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(am::packet_variant(subscribe), as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(am::packet_variant(unsubscribe), as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(am::packet_variant(pingreq), as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    // register packet_id for testing
    BOOST_TEST(ep.register_packet_id(0x1, as::use_future).get());
    // offline publish success
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(publish, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::success);
    }

    // runtime error due to send before receiving connect
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(puback, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(pubrec, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(pubrel, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(pubcomp, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(suback, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(unsuback, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(pingresp, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    // recv connect
    {
        auto pv = ep.recv(as::use_future).get();
        BOOST_TEST(am::packet_compare(connect, pv));
    }

    // offline publish success
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(publish, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::success);
    }

    // runtime error due to send before sending connack
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(puback, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(pubrec, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(pubrel, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(pubcomp, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(suback, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(unsuback, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(pingresp, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
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

    // offline publish success
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(publish, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::success);
    }

    // runtime error due to send before receiving connack with not_authorized
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(puback, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(pubrec, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(pubrel, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(pubcomp, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(suback, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(unsuback, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(pingresp, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }
    ep.close(as::use_future).get();
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

    am::endpoint<async_mqtt::role::client, async_mqtt::stub_socket> ep{
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
        am::properties{}
    };

    auto disconnect = am::v5::disconnect_packet{
        am::disconnect_reason_code::normal_disconnection,
        am::properties{}
    };

    auto publish = am::v5::publish_packet(
        0x1, // hard coded packet_id for just testing
        am::allocate_buffer("topic1"),
        am::allocate_buffer("payload1"),
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
            {am::allocate_buffer("topic1"), am::qos::at_most_once},
            {am::allocate_buffer("topic2"), am::qos::exactly_once},
        },
        am::properties{}
    };

    auto unsubscribe = am::v5::unsubscribe_packet{
        0x1, // hard coded packet_id for just testing
        std::vector<am::topic_sharename> {
            am::allocate_buffer("topic1"),
            am::allocate_buffer("topic2"),
        },
        am::properties{}
    };

    auto pingreq = am::v5::pingreq_packet();

    auto auth = am::v5::auth_packet{
        am::auth_reason_code::continue_authentication,
        am::properties{}
    };

    auto close = am::make_error(am::errc::network_reset, "pseudo close");

    ep.next_layer().set_recv_packets(
        {
            // receive packets
            connack,
            close,
            connack,
            close,
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

    // send auth packet that can be sent before connack receiving
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(auth, wp));
        }
    );
    {
        auto ec = ep.send(auth, as::use_future).get();
        BOOST_TEST(!ec);
    }

    // recv connack
    {
        auto pv = ep.recv(as::use_future).get();
        BOOST_TEST(am::packet_compare(connack, pv));
    }

    // send valid packets
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(auth, wp));
        }
    );
    {
        auto ec = ep.send(auth, as::use_future).get();
        BOOST_TEST(!ec);
    }

    // register packet_id for testing
    BOOST_TEST(ep.register_packet_id(0x1, as::use_future).get());
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(publish, wp));
        }
    );
    {
        auto ec = ep.send(publish, as::use_future).get();
        BOOST_TEST(!ec);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(puback, wp));
        }
    );
    {
        auto ec = ep.send(puback, as::use_future).get();
        BOOST_TEST(!ec);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(pubrec, wp));
        }
    );
    {
        auto ec = ep.send(pubrec, as::use_future).get();
        BOOST_TEST(!ec);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(pubrel, wp));
        }
    );
    {
        auto ec = ep.send(pubrel, as::use_future).get();
        BOOST_TEST(!ec);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(pubcomp, wp));
        }
    );
    {
        auto ec = ep.send(pubcomp, as::use_future).get();
        BOOST_TEST(!ec);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(subscribe, wp));
        }
    );
    {
        auto ec = ep.send(subscribe, as::use_future).get();
        BOOST_TEST(!ec);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(unsubscribe, wp));
        }
    );
    {
        auto ec = ep.send(unsubscribe, as::use_future).get();
        BOOST_TEST(!ec);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(pingreq, wp));
        }
    );
    {
        auto ec = ep.send(pingreq, as::use_future).get();
        BOOST_TEST(!ec);
    }


    // send disconnect
    ep.next_layer().set_write_packet_checker(
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

    // send disconnect
    ep.next_layer().set_write_packet_checker(
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

BOOST_AUTO_TEST_CASE(invalid_client_v5) {
    auto version = am::protocol_version::v5;
    as::io_context ioc;
    auto guard = as::make_work_guard(ioc.get_executor());
    std::thread th {
        [&] {
            ioc.run();
        }
    };

    am::endpoint<async_mqtt::role::client, async_mqtt::stub_socket> ep{
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
        am::allocate_buffer("topic1"),
        am::allocate_buffer("payload1"),
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
            {am::allocate_buffer("topic1"), am::qos::at_most_once},
            {am::allocate_buffer("topic2"), am::qos::exactly_once},
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
            am::allocate_buffer("topic1"),
            am::allocate_buffer("topic2"),
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

    auto close = am::make_error(am::errc::network_reset, "pseudo close");

    ep.next_layer().set_recv_packets(
        {
            // receive packets
            connack
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
        auto ec = ep.send(connack, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(suback, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(unsuback, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(pingresp, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

#endif // defined(ASYNC_MQTT_TEST_COMPILE_ERROR)

    // runtime error due to unable send packet
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(am::packet_variant(connack), as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(am::packet_variant(suback), as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(am::packet_variant(unsuback), as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(am::packet_variant(pingresp), as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    // register packet_id for testing
    BOOST_TEST(ep.register_packet_id(0x1, as::use_future).get());
    // offline publish success
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(publish, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::success);
    }

    // runtime error due to send before sending connect
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(puback, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(pubrec, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(pubrel, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(pubcomp, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(subscribe, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(unsubscribe, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(pingreq, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(auth, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

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

    // register packet_id for testing
    BOOST_TEST(ep.register_packet_id(0x1, as::use_future).get());
    // offline publish success
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(publish, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::success);
    }

    // runtime error due to send before receiving connack
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(puback, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(pubrec, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(pubrel, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(pubcomp, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(subscribe, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(unsubscribe, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.next_layer().set_write_packet_checker(
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

    // offline publish success
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(publish, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::success);
    }

    // runtime error due to send before receiving connack with not_authorized
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(puback, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(pubrec, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(pubrel, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(pubcomp, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(subscribe, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(unsubscribe, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(pingreq, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }
    ep.close(as::use_future).get();
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

    am::endpoint<async_mqtt::role::server, async_mqtt::stub_socket> ep1{
        version,
        // for stub_socket args
        version,
        ioc
    };

    am::endpoint<async_mqtt::role::server, async_mqtt::stub_socket> ep2{
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
        am::properties{}
    };

    auto disconnect = am::v5::disconnect_packet{
        am::disconnect_reason_code::normal_disconnection,
        am::properties{}
    };

    auto publish = am::v5::publish_packet(
        0x1, // hard coded packet_id for just testing
        am::allocate_buffer("topic1"),
        am::allocate_buffer("payload1"),
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

    auto close = am::make_error(am::errc::network_reset, "pseudo close");

    ep1.next_layer().set_recv_packets(
        {
            // receive packets
            connect,
            close,
        }
    );

    // recv connect
    {
        auto pv = ep1.recv(as::use_future).get();
        BOOST_TEST(am::packet_compare(connect, pv));
    }

    // send auth
    ep1.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(auth, wp));
        }
    );
    {
        auto ec = ep1.send(auth, as::use_future).get();
        BOOST_TEST(!ec);
    }

    // send connack
    ep1.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(connack, wp));
        }
    );
    {
        auto ec = ep1.send(connack, as::use_future).get();
        BOOST_TEST(!ec);
    }

    // register packet_id for testing
    BOOST_TEST(ep1.register_packet_id(0x1, as::use_future).get());

    // send valid packets
    ep1.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(auth, wp));
        }
    );
    {
        auto ec = ep1.send(auth, as::use_future).get();
        BOOST_TEST(!ec);
    }

    ep1.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(publish, wp));
        }
    );
    {
        auto ec = ep1.send(publish, as::use_future).get();
        BOOST_TEST(!ec);
    }

    ep1.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(puback, wp));
        }
    );
    {
        auto ec = ep1.send(puback, as::use_future).get();
        BOOST_TEST(!ec);
    }

    ep1.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(pubrec, wp));
        }
    );
    {
        auto ec = ep1.send(pubrec, as::use_future).get();
        BOOST_TEST(!ec);
    }

    ep1.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(pubrel, wp));
        }
    );
    {
        auto ec = ep1.send(pubrel, as::use_future).get();
        BOOST_TEST(!ec);
    }

    ep1.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(pubcomp, wp));
        }
    );
    {
        auto ec = ep1.send(pubcomp, as::use_future).get();
        BOOST_TEST(!ec);
    }

    ep1.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(suback, wp));
        }
    );
    {
        auto ec = ep1.send(suback, as::use_future).get();
        BOOST_TEST(!ec);
    }

    ep1.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(unsuback, wp));
        }
    );
    {
        auto ec = ep1.send(unsuback, as::use_future).get();
        BOOST_TEST(!ec);
    }

    ep1.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(pingresp, wp));
        }
    );
    {
        auto ec = ep1.send(pingresp, as::use_future).get();
        BOOST_TEST(!ec);
    }


    // recv close
    {
        auto pv = ep1.recv(as::use_future).get();
        BOOST_TEST(pv.get_if<am::system_error>() != nullptr);
    }

    ep2.next_layer().set_recv_packets(
        {
            // receive packets
            connect,
            close,
        }
    );

    // recv connect
    {
        auto pv = ep2.recv(as::use_future).get();
        BOOST_TEST(am::packet_compare(connect, pv));
    }

    // send connack
    ep2.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(connack, wp));
        }
    );
    {
        auto ec = ep2.send(connack, as::use_future).get();
        BOOST_TEST(!ec);
    }

    // register packet_id for testing
    BOOST_TEST(ep2.register_packet_id(0x1, as::use_future).get());

    // send publish behalf of valid packets
    ep2.next_layer().set_write_packet_checker(
        [&](am::packet_variant wp) {
            BOOST_TEST(am::packet_compare(publish, wp));
        }
    );
    {
        auto ec = ep2.send(publish, as::use_future).get();
        BOOST_TEST(!ec);
    }

    // recv close
    {
        auto pv = ep2.recv(as::use_future).get();
        BOOST_TEST(pv.get_if<am::system_error>() != nullptr);
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

    am::endpoint<async_mqtt::role::server, async_mqtt::stub_socket> ep{
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
        am::allocate_buffer("topic1"),
        am::allocate_buffer("payload1"),
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
            {am::allocate_buffer("topic1"), am::qos::at_most_once},
            {am::allocate_buffer("topic2"), am::qos::exactly_once},
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
            am::allocate_buffer("topic1"),
            am::allocate_buffer("topic2"),
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

    auto close = am::make_error(am::errc::network_reset, "pseudo close");

    ep.next_layer().set_recv_packets(
        {
            // receive packets
            connect
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
        auto ec = ep.send(connect, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(subscribe, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(unsubscribe, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(pingreq, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

#endif // defined(ASYNC_MQTT_TEST_COMPILE_ERROR)

    // runtime error due to unable send packet
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(am::packet_variant(connect), as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(am::packet_variant(subscribe), as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(am::packet_variant(unsubscribe), as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(am::packet_variant(pingreq), as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    // register packet_id for testing
    BOOST_TEST(ep.register_packet_id(0x1, as::use_future).get());
    // offline publish success
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(publish, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::success);
    }

    // runtime error due to send before receiving connect
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(auth, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(puback, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(pubrec, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(pubrel, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(pubcomp, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(suback, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(unsuback, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(pingresp, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    // recv connect
    {
        auto pv = ep.recv(as::use_future).get();
        BOOST_TEST(am::packet_compare(connect, pv));
    }

    // offline publish success
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(publish, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::success);
    }

    // runtime error due to send before sending connack
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(puback, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(pubrec, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(pubrel, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(pubcomp, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(suback, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(unsuback, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(pingresp, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
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

    // offline publish success
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(publish, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::success);
    }

    // runtime error due to send before receiving connack with not_authorized
    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(puback, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(pubrec, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(pubrel, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(pubcomp, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(suback, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(unsuback, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }

    ep.next_layer().set_write_packet_checker(
        [&](am::packet_variant) {
            BOOST_TEST(false);
        }
    );
    {
        auto ec = ep.send(pingresp, as::use_future).get();
        BOOST_TEST(ec.code() == am::errc::protocol_error);
    }
    ep.close(as::use_future).get();
    guard.reset();
    th.join();
}

BOOST_AUTO_TEST_SUITE_END()
