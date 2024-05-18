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
#include <async_mqtt/util/packet_variant_operator.hpp>

BOOST_AUTO_TEST_SUITE(ut_ep_send_rt_chk)

namespace am = async_mqtt;
namespace as = boost::asio;

using namespace std::literals::string_view_literals;

inline bool ec_what_start_with(am::system_error const& se, std::string_view sv) {
    if (!se) return false;
    if (std::string_view(se.what()).find(sv) == 0) return true;
    return false;
}

BOOST_AUTO_TEST_CASE(v311_client) {
    auto version = am::protocol_version::v3_1_1;
    as::io_context ioc;
    auto guard = as::make_work_guard(ioc.get_executor());
    std::thread th {
        [&] {
            ioc.run();
        }
    };
    auto ep = am::endpoint<async_mqtt::role::client, async_mqtt::stub_socket>::create(
        version,
        // for stub_socket args
        version,
        ioc.get_executor()
    );

    {
        auto p = am::v5::connect_packet{
            true,   // clean_start
            0x1234, // keep_alive
            "cid1",
            std::nullopt,
            "user1",
            "pass1",
            am::properties{}
        };
        auto ec = ep->async_send(am::packet_variant{p}, as::use_future).get();
        BOOST_TEST(ec_what_start_with(ec, "protocol version mismatch"));
    }
    {
        auto p = am::v3_1_1::connect_packet{
            true,   // clean_session
            0x0, // keep_alive
            "cid1",
            std::nullopt,
            "user1",
            "pass1"
        };
        auto ec = ep->async_send(am::packet_variant{p}, as::use_future).get();
        BOOST_TEST(!ec_what_start_with(ec, "packet cannot be send by MQTT protocol"));
    }
    {
        auto p = am::v3_1_1::connack_packet{
            true,   // session_present
            am::connect_return_code::not_authorized
        };
        auto ec = ep->async_send(am::packet_variant{p}, as::use_future).get();
        BOOST_TEST(ec_what_start_with(ec, "packet cannot be send by MQTT protocol"));
    }
    {
        auto p = am::v3_1_1::publish_packet{
            "topic1",
            "payload1",
            am::qos::at_most_once | am::pub::retain::yes | am::pub::dup::yes
        };
        auto ec = ep->async_send(am::packet_variant{p}, as::use_future).get();
        BOOST_TEST(!ec_what_start_with(ec, "packet cannot be send by MQTT protocol"));
    }
    {
        auto p = am::v3_1_1::puback_packet{
            0x1234 // packet_id
        };
        auto ec = ep->async_send(am::packet_variant{p}, as::use_future).get();
        BOOST_TEST(!ec_what_start_with(ec, "packet cannot be send by MQTT protocol"));
    }
    {
        auto p = am::v3_1_1::pubrec_packet{
            0x1234 // packet_id
        };
        auto ec = ep->async_send(am::packet_variant{p}, as::use_future).get();
        BOOST_TEST(!ec_what_start_with(ec, "packet cannot be send by MQTT protocol"));
    }
    {
        auto p = am::v3_1_1::pubrel_packet(
            0x1234 // packet_id
        );
        auto ec = ep->async_send(am::packet_variant{p}, as::use_future).get();
        BOOST_TEST(!ec_what_start_with(ec, "packet cannot be send by MQTT protocol"));
    }
    {
        auto p = am::v3_1_1::pubcomp_packet{
            0x1234 // packet_id
        };
        auto ec = ep->async_send(am::packet_variant{p}, as::use_future).get();
        BOOST_TEST(!ec_what_start_with(ec, "packet cannot be send by MQTT protocol"));
    }
    {
        std::vector<am::topic_subopts> args {
            {"topic1", am::qos::at_most_once},
            {"topic2", am::qos::exactly_once},
        };
        auto p = am::v3_1_1::subscribe_packet{
            0x1234,         // packet_id
            args
        };
        auto ec = ep->async_send(am::packet_variant{p}, as::use_future).get();
        BOOST_TEST(!ec_what_start_with(ec, "packet cannot be send by MQTT protocol"));
    }
    {
        std::vector<am::suback_return_code> args {
            am::suback_return_code::success_maximum_qos_1,
            am::suback_return_code::failure
        };
        auto p = am::v3_1_1::suback_packet{
            0x1234,         // packet_id
            args
        };
        auto ec = ep->async_send(am::packet_variant{p}, as::use_future).get();
        BOOST_TEST(ec_what_start_with(ec, "packet cannot be send by MQTT protocol"));
    }
    {
        std::vector<am::topic_sharename> args {
            {"topic1"},
            {"topic2"},
        };

        auto p = am::v3_1_1::unsubscribe_packet{
            0x1234,         // packet_id
            args
        };
        auto ec = ep->async_send(am::packet_variant{p}, as::use_future).get();
        BOOST_TEST(!ec_what_start_with(ec, "packet cannot be send by MQTT protocol"));
    }
    {
        auto p = am::v3_1_1::unsuback_packet{
            0x1234 // packet_id
        };
        auto ec = ep->async_send(am::packet_variant{p}, as::use_future).get();
        BOOST_TEST(ec_what_start_with(ec, "packet cannot be send by MQTT protocol"));
    }
    {
        auto p = am::v3_1_1::pingreq_packet{};
        auto ec = ep->async_send(am::packet_variant{p}, as::use_future).get();
        BOOST_TEST(!ec_what_start_with(ec, "packet cannot be send by MQTT protocol"));
    }
    {
        auto p = am::v3_1_1::pingresp_packet{};
        auto ec = ep->async_send(am::packet_variant{p}, as::use_future).get();
        BOOST_TEST(ec_what_start_with(ec, "packet cannot be send by MQTT protocol"));
    }
    {
        auto p = am::v3_1_1::disconnect_packet();
        auto ec = ep->async_send(am::packet_variant{p}, as::use_future).get();
        BOOST_TEST(!ec_what_start_with(ec, "packet cannot be send by MQTT protocol"));
    }

    guard.reset();
    th.join();
}

BOOST_AUTO_TEST_CASE(v311_server) {
    auto version = am::protocol_version::v3_1_1;
    as::io_context ioc;
    auto guard = as::make_work_guard(ioc.get_executor());
    std::thread th {
        [&] {
            ioc.run();
        }
    };
    auto ep = am::endpoint<async_mqtt::role::server, async_mqtt::stub_socket>::create(
        version,
        // for stub_socket args
        version,
        ioc.get_executor()
    );

    {
        auto p = am::v3_1_1::connect_packet{
            true,   // clean_session
            0x0, // keep_alive
            "cid1",
            std::nullopt,
            "user1",
            "pass1"
        };
        auto ec = ep->async_send(am::packet_variant{p}, as::use_future).get();
        BOOST_TEST(ec_what_start_with(ec, "packet cannot be send by MQTT protocol"));
    }
    {
        auto p = am::v3_1_1::connack_packet{
            true,   // session_present
            am::connect_return_code::not_authorized
        };
        auto ec = ep->async_send(am::packet_variant{p}, as::use_future).get();
        BOOST_TEST(!ec_what_start_with(ec, "packet cannot be send by MQTT protocol"));
    }
    {
        auto p = am::v3_1_1::publish_packet{
            "topic1",
            "payload1",
            am::qos::at_most_once | am::pub::retain::yes | am::pub::dup::yes
        };
        auto ec = ep->async_send(am::packet_variant{p}, as::use_future).get();
        BOOST_TEST(!ec_what_start_with(ec, "packet cannot be send by MQTT protocol"));
    }
    {
        auto p = am::v3_1_1::puback_packet{
            0x1234 // packet_id
        };
        auto ec = ep->async_send(am::packet_variant{p}, as::use_future).get();
        BOOST_TEST(!ec_what_start_with(ec, "packet cannot be send by MQTT protocol"));
    }
    {
        auto p = am::v3_1_1::pubrec_packet{
            0x1234 // packet_id
        };
        auto ec = ep->async_send(am::packet_variant{p}, as::use_future).get();
        BOOST_TEST(!ec_what_start_with(ec, "packet cannot be send by MQTT protocol"));
    }
    {
        auto p = am::v3_1_1::pubrel_packet(
            0x1234 // packet_id
        );
        auto ec = ep->async_send(am::packet_variant{p}, as::use_future).get();
        BOOST_TEST(!ec_what_start_with(ec, "packet cannot be send by MQTT protocol"));
    }
    {
        auto p = am::v3_1_1::pubcomp_packet{
            0x1234 // packet_id
        };
        auto ec = ep->async_send(am::packet_variant{p}, as::use_future).get();
        BOOST_TEST(!ec_what_start_with(ec, "packet cannot be send by MQTT protocol"));
    }
    {
        std::vector<am::topic_subopts> args {
            {"topic1", am::qos::at_most_once},
            {"topic2", am::qos::exactly_once},
        };
        auto p = am::v3_1_1::subscribe_packet{
            0x1234,         // packet_id
            args
        };
        auto ec = ep->async_send(am::packet_variant{p}, as::use_future).get();
        BOOST_TEST(ec_what_start_with(ec, "packet cannot be send by MQTT protocol"));
    }
    {
        std::vector<am::suback_return_code> args {
            am::suback_return_code::success_maximum_qos_1,
            am::suback_return_code::failure
        };
        auto p = am::v3_1_1::suback_packet{
            0x1234,         // packet_id
            args
        };
        auto ec = ep->async_send(am::packet_variant{p}, as::use_future).get();
        BOOST_TEST(!ec_what_start_with(ec, "packet cannot be send by MQTT protocol"));
    }
    {
        std::vector<am::topic_sharename> args {
            {"topic1"},
            {"topic2"},
        };

        auto p = am::v3_1_1::unsubscribe_packet{
            0x1234,         // packet_id
            args
        };
        auto ec = ep->async_send(am::packet_variant{p}, as::use_future).get();
        BOOST_TEST(ec_what_start_with(ec, "packet cannot be send by MQTT protocol"));
    }
    {
        auto p = am::v3_1_1::unsuback_packet{
            0x1234 // packet_id
        };
        auto ec = ep->async_send(am::packet_variant{p}, as::use_future).get();
        BOOST_TEST(!ec_what_start_with(ec, "packet cannot be send by MQTT protocol"));
    }
    {
        auto p = am::v3_1_1::pingreq_packet{};
        auto ec = ep->async_send(am::packet_variant{p}, as::use_future).get();
        BOOST_TEST(ec_what_start_with(ec, "packet cannot be send by MQTT protocol"));
    }
    {
        auto p = am::v3_1_1::pingresp_packet{};
        auto ec = ep->async_send(am::packet_variant{p}, as::use_future).get();
        BOOST_TEST(!ec_what_start_with(ec, "packet cannot be send by MQTT protocol"));
    }
    {
        auto p = am::v3_1_1::disconnect_packet();
        auto ec = ep->async_send(am::packet_variant{p}, as::use_future).get();
        BOOST_TEST(ec_what_start_with(ec, "packet cannot be send by MQTT protocol"));
    }

    guard.reset();
    th.join();
}


BOOST_AUTO_TEST_CASE(v311_any) {
    auto version = am::protocol_version::v3_1_1;
    as::io_context ioc;
    auto guard = as::make_work_guard(ioc.get_executor());
    std::thread th {
        [&] {
            ioc.run();
        }
    };
    auto ep = am::endpoint<async_mqtt::role::any, async_mqtt::stub_socket>::create(
        version,
        // for stub_socket args
        version,
        ioc.get_executor()
    );

    {
        auto p = am::v3_1_1::connect_packet{
            true,   // clean_session
            0x0, // keep_alive
            "cid1",
            std::nullopt,
            "user1",
            "pass1"
        };
        auto ec = ep->async_send(am::packet_variant{p}, as::use_future).get();
        BOOST_TEST(!ec_what_start_with(ec, "packet cannot be send by MQTT protocol"));
    }
    {
        auto p = am::v3_1_1::connack_packet{
            true,   // session_present
            am::connect_return_code::not_authorized
        };
        auto ec = ep->async_send(am::packet_variant{p}, as::use_future).get();
        BOOST_TEST(!ec_what_start_with(ec, "packet cannot be send by MQTT protocol"));
    }
    {
        auto p = am::v3_1_1::publish_packet{
            "topic1",
            "payload1",
            am::qos::at_most_once | am::pub::retain::yes | am::pub::dup::yes
        };
        auto ec = ep->async_send(am::packet_variant{p}, as::use_future).get();
        BOOST_TEST(!ec_what_start_with(ec, "packet cannot be send by MQTT protocol"));
    }
    {
        auto p = am::v3_1_1::puback_packet{
            0x1234 // packet_id
        };
        auto ec = ep->async_send(am::packet_variant{p}, as::use_future).get();
        BOOST_TEST(!ec_what_start_with(ec, "packet cannot be send by MQTT protocol"));
    }
    {
        auto p = am::v3_1_1::pubrec_packet{
            0x1234 // packet_id
        };
        auto ec = ep->async_send(am::packet_variant{p}, as::use_future).get();
        BOOST_TEST(!ec_what_start_with(ec, "packet cannot be send by MQTT protocol"));
    }
    {
        auto p = am::v3_1_1::pubrel_packet(
            0x1234 // packet_id
        );
        auto ec = ep->async_send(am::packet_variant{p}, as::use_future).get();
        BOOST_TEST(!ec_what_start_with(ec, "packet cannot be send by MQTT protocol"));
    }
    {
        auto p = am::v3_1_1::pubcomp_packet{
            0x1234 // packet_id
        };
        auto ec = ep->async_send(am::packet_variant{p}, as::use_future).get();
        BOOST_TEST(!ec_what_start_with(ec, "packet cannot be send by MQTT protocol"));
    }
    {
        std::vector<am::topic_subopts> args {
            {"topic1", am::qos::at_most_once},
            {"topic2", am::qos::exactly_once},
        };
        auto p = am::v3_1_1::subscribe_packet{
            0x1234,         // packet_id
            args
        };
        auto ec = ep->async_send(am::packet_variant{p}, as::use_future).get();
        BOOST_TEST(!ec_what_start_with(ec, "packet cannot be send by MQTT protocol"));
    }
    {
        std::vector<am::suback_return_code> args {
            am::suback_return_code::success_maximum_qos_1,
            am::suback_return_code::failure
        };
        auto p = am::v3_1_1::suback_packet{
            0x1234,         // packet_id
            args
        };
        auto ec = ep->async_send(am::packet_variant{p}, as::use_future).get();
        BOOST_TEST(!ec_what_start_with(ec, "packet cannot be send by MQTT protocol"));
    }
    {
        std::vector<am::topic_sharename> args {
            {"topic1"},
            {"topic2"},
        };

        auto p = am::v3_1_1::unsubscribe_packet{
            0x1234,         // packet_id
            args
        };
        auto ec = ep->async_send(am::packet_variant{p}, as::use_future).get();
        BOOST_TEST(!ec_what_start_with(ec, "packet cannot be send by MQTT protocol"));
    }
    {
        auto p = am::v3_1_1::unsuback_packet{
            0x1234 // packet_id
        };
        auto ec = ep->async_send(am::packet_variant{p}, as::use_future).get();
        BOOST_TEST(!ec_what_start_with(ec, "packet cannot be send by MQTT protocol"));
    }
    {
        auto p = am::v3_1_1::pingreq_packet{};
        auto ec = ep->async_send(am::packet_variant{p}, as::use_future).get();
        BOOST_TEST(!ec_what_start_with(ec, "packet cannot be send by MQTT protocol"));
    }
    {
        auto p = am::v3_1_1::pingresp_packet{};
        auto ec = ep->async_send(am::packet_variant{p}, as::use_future).get();
        BOOST_TEST(!ec_what_start_with(ec, "packet cannot be send by MQTT protocol"));
    }
    {
        auto p = am::v3_1_1::disconnect_packet();
        auto ec = ep->async_send(am::packet_variant{p}, as::use_future).get();
        BOOST_TEST(!ec_what_start_with(ec, "packet cannot be send by MQTT protocol"));
    }

    guard.reset();
    th.join();
}

BOOST_AUTO_TEST_CASE(v5_client) {
    auto version = am::protocol_version::v5;
    as::io_context ioc;
    auto guard = as::make_work_guard(ioc.get_executor());
    std::thread th {
        [&] {
            ioc.run();
        }
    };
    auto ep = am::endpoint<async_mqtt::role::client, async_mqtt::stub_socket>::create(
        version,
        // for stub_socket args
        version,
        ioc.get_executor()
    );

    {
        auto p = am::v3_1_1::connect_packet{
            true,   // clean_session
            0x0, // keep_alive
            "cid1",
            std::nullopt,
            "user1",
            "pass1"
        };
        auto ec = ep->async_send(am::packet_variant{p}, as::use_future).get();
        BOOST_TEST(ec_what_start_with(ec, "protocol version mismatch"));
    }
    {
        auto p = am::v5::connect_packet{
            true,   // clean_start
            0x0, // keep_alive
            "cid1",
            std::nullopt,
            "user1",
            "pass1",
            am::properties{}
        };
        auto ec = ep->async_send(am::packet_variant{p}, as::use_future).get();
        BOOST_TEST(!ec_what_start_with(ec, "packet cannot be send by MQTT protocol"));
    }
    {
        auto p = am::v5::connack_packet{
            true,   // session_present
            am::connect_reason_code::not_authorized,
            am::properties{}
        };
        auto ec = ep->async_send(am::packet_variant{p}, as::use_future).get();
        // packet type checking is prior to version checking
        BOOST_TEST(ec_what_start_with(ec, "packet cannot be send by MQTT protocol"));
    }
    {
        auto p = am::v5::publish_packet{
            "topic1",
            "payload1",
            am::qos::at_most_once | am::pub::retain::yes | am::pub::dup::yes,
            am::properties{}
        };
        auto ec = ep->async_send(am::packet_variant{p}, as::use_future).get();
        BOOST_TEST(!ec_what_start_with(ec, "packet cannot be send by MQTT protocol"));
    }
    {
        auto p = am::v5::puback_packet{
            0x1234, // packet_id
            am::puback_reason_code::packet_identifier_in_use,
            am::properties{}
        };
        auto ec = ep->async_send(am::packet_variant{p}, as::use_future).get();
        BOOST_TEST(!ec_what_start_with(ec, "packet cannot be send by MQTT protocol"));
    }
    {
        auto p = am::v5::pubrec_packet{
            0x1234, // packet_id
            am::pubrec_reason_code::packet_identifier_in_use,
            am::properties{}
        };
        auto ec = ep->async_send(am::packet_variant{p}, as::use_future).get();
        BOOST_TEST(!ec_what_start_with(ec, "packet cannot be send by MQTT protocol"));
    }
    {
        auto p = am::v5::pubrel_packet{
            0x1234, // packet_id
            am::pubrel_reason_code::packet_identifier_not_found,
            am::properties{}
        };
        auto ec = ep->async_send(am::packet_variant{p}, as::use_future).get();
        BOOST_TEST(!ec_what_start_with(ec, "packet cannot be send by MQTT protocol"));
    }
    {
        auto p = am::v5::pubcomp_packet{
            0x1234, // packet_id
            am::pubcomp_reason_code::packet_identifier_not_found,
            am::properties{}
        };
        auto ec = ep->async_send(am::packet_variant{p}, as::use_future).get();
        BOOST_TEST(!ec_what_start_with(ec, "packet cannot be send by MQTT protocol"));
    }
    {
        std::vector<am::topic_subopts> args {
            {"topic1", am::qos::at_most_once | am::sub::nl::yes | am::sub::retain_handling::not_send},
            {"topic2", am::qos::exactly_once | am::sub::rap::retain},
        };
        auto p = am::v5::subscribe_packet{
            0x1234,         // packet_id
            args,
            am::properties{}
        };
        auto ec = ep->async_send(am::packet_variant{p}, as::use_future).get();
        BOOST_TEST(!ec_what_start_with(ec, "packet cannot be send by MQTT protocol"));
    }
    {
        std::vector<am::suback_reason_code> args {
            am::suback_reason_code::granted_qos_1,
            am::suback_reason_code::unspecified_error
        };
        auto p = am::v5::suback_packet{
            0x1234,         // packet_id
            args,
            am::properties{}
        };
        auto ec = ep->async_send(am::packet_variant{p}, as::use_future).get();
        BOOST_TEST(ec_what_start_with(ec, "packet cannot be send by MQTT protocol"));
    }
    {
        std::vector<am::topic_sharename> args {
            {"topic1"},
            {"topic2"},
        };
        auto p = am::v5::unsubscribe_packet{
            0x1234,         // packet_id
            args,
            am::properties{}
        };
        auto ec = ep->async_send(am::packet_variant{p}, as::use_future).get();
        BOOST_TEST(!ec_what_start_with(ec, "packet cannot be send by MQTT protocol"));
    }
    {
        std::vector<am::unsuback_reason_code> args {
            am::unsuback_reason_code::no_subscription_existed,
            am::unsuback_reason_code::unspecified_error
        };
        auto p = am::v5::unsuback_packet{
            0x1234,         // packet_id
            args,
            am::properties{}
        };
        auto ec = ep->async_send(am::packet_variant{p}, as::use_future).get();
        BOOST_TEST(ec_what_start_with(ec, "packet cannot be send by MQTT protocol"));
    }
    {
        auto p = am::v5::pingreq_packet{};
        auto ec = ep->async_send(am::packet_variant{p}, as::use_future).get();
        BOOST_TEST(!ec_what_start_with(ec, "packet cannot be send by MQTT protocol"));
    }
    {
        auto p = am::v5::pingresp_packet{};
        auto ec = ep->async_send(am::packet_variant{p}, as::use_future).get();
        BOOST_TEST(ec_what_start_with(ec, "packet cannot be send by MQTT protocol"));
    }
    {
        auto p = am::v5::auth_packet{
            am::auth_reason_code::continue_authentication,
            am::properties{}
        };
        auto ec = ep->async_send(am::packet_variant{p}, as::use_future).get();
        BOOST_TEST(!ec_what_start_with(ec, "packet cannot be send by MQTT protocol"));
    }
    {
        auto p = am::v5::disconnect_packet{
            am::disconnect_reason_code::protocol_error,
            am::properties{}
        };
        auto ec = ep->async_send(am::packet_variant{p}, as::use_future).get();
        BOOST_TEST(!ec_what_start_with(ec, "packet cannot be send by MQTT protocol"));
    }

    guard.reset();
    th.join();
}

BOOST_AUTO_TEST_CASE(v5_server) {
    auto version = am::protocol_version::v5;
    as::io_context ioc;
    auto guard = as::make_work_guard(ioc.get_executor());
    std::thread th {
        [&] {
            ioc.run();
        }
    };
    auto ep = am::endpoint<async_mqtt::role::server, async_mqtt::stub_socket>::create(
        version,
        // for stub_socket args
        version,
        ioc.get_executor()
    );

    {
        auto p = am::v5::connect_packet{
            true,   // clean_start
            0x0, // keep_alive
            "cid1",
            std::nullopt,
            "user1",
            "pass1",
            am::properties{}
        };
        auto ec = ep->async_send(am::packet_variant{p}, as::use_future).get();
        BOOST_TEST(ec_what_start_with(ec, "packet cannot be send by MQTT protocol"));
    }
    {
        auto p = am::v5::connack_packet{
            true,   // session_present
            am::connect_reason_code::not_authorized,
            am::properties{}
        };
        auto ec = ep->async_send(am::packet_variant{p}, as::use_future).get();
        // packet type checking is prior to version checking
        BOOST_TEST(!ec_what_start_with(ec, "packet cannot be send by MQTT protocol"));
    }
    {
        auto p = am::v5::publish_packet{
            "topic1",
            "payload1",
            am::qos::at_most_once | am::pub::retain::yes | am::pub::dup::yes,
            am::properties{}
        };
        auto ec = ep->async_send(am::packet_variant{p}, as::use_future).get();
        BOOST_TEST(!ec_what_start_with(ec, "packet cannot be send by MQTT protocol"));
    }
    {
        auto p = am::v5::puback_packet{
            0x1234, // packet_id
            am::puback_reason_code::packet_identifier_in_use,
            am::properties{}
        };
        auto ec = ep->async_send(am::packet_variant{p}, as::use_future).get();
        BOOST_TEST(!ec_what_start_with(ec, "packet cannot be send by MQTT protocol"));
    }
    {
        auto p = am::v5::pubrec_packet{
            0x1234, // packet_id
            am::pubrec_reason_code::packet_identifier_in_use,
            am::properties{}
        };
        auto ec = ep->async_send(am::packet_variant{p}, as::use_future).get();
        BOOST_TEST(!ec_what_start_with(ec, "packet cannot be send by MQTT protocol"));
    }
    {
        auto p = am::v5::pubrel_packet{
            0x1234, // packet_id
            am::pubrel_reason_code::packet_identifier_not_found,
            am::properties{}
        };
        auto ec = ep->async_send(am::packet_variant{p}, as::use_future).get();
        BOOST_TEST(!ec_what_start_with(ec, "packet cannot be send by MQTT protocol"));
    }
    {
        auto p = am::v5::pubcomp_packet{
            0x1234, // packet_id
            am::pubcomp_reason_code::packet_identifier_not_found,
            am::properties{}
        };
        auto ec = ep->async_send(am::packet_variant{p}, as::use_future).get();
        BOOST_TEST(!ec_what_start_with(ec, "packet cannot be send by MQTT protocol"));
    }
    {
        std::vector<am::topic_subopts> args {
            {"topic1", am::qos::at_most_once | am::sub::nl::yes | am::sub::retain_handling::not_send},
            {"topic2", am::qos::exactly_once | am::sub::rap::retain},
        };
        auto p = am::v5::subscribe_packet{
            0x1234,         // packet_id
            args,
            am::properties{}
        };
        auto ec = ep->async_send(am::packet_variant{p}, as::use_future).get();
        BOOST_TEST(ec_what_start_with(ec, "packet cannot be send by MQTT protocol"));
    }
    {
        std::vector<am::suback_reason_code> args {
            am::suback_reason_code::granted_qos_1,
            am::suback_reason_code::unspecified_error
        };
        auto p = am::v5::suback_packet{
            0x1234,         // packet_id
            args,
            am::properties{}
        };
        auto ec = ep->async_send(am::packet_variant{p}, as::use_future).get();
        BOOST_TEST(!ec_what_start_with(ec, "packet cannot be send by MQTT protocol"));
    }
    {
        std::vector<am::topic_sharename> args {
            {"topic1"},
            {"topic2"},
        };
        auto p = am::v5::unsubscribe_packet{
            0x1234,         // packet_id
            args,
            am::properties{}
        };
        auto ec = ep->async_send(am::packet_variant{p}, as::use_future).get();
        BOOST_TEST(ec_what_start_with(ec, "packet cannot be send by MQTT protocol"));
    }
    {
        std::vector<am::unsuback_reason_code> args {
            am::unsuback_reason_code::no_subscription_existed,
            am::unsuback_reason_code::unspecified_error
        };
        auto p = am::v5::unsuback_packet{
            0x1234,         // packet_id
            args,
            am::properties{}
        };
        auto ec = ep->async_send(am::packet_variant{p}, as::use_future).get();
        BOOST_TEST(!ec_what_start_with(ec, "packet cannot be send by MQTT protocol"));
    }
    {
        auto p = am::v5::pingreq_packet{};
        auto ec = ep->async_send(am::packet_variant{p}, as::use_future).get();
        BOOST_TEST(ec_what_start_with(ec, "packet cannot be send by MQTT protocol"));
    }
    {
        auto p = am::v5::pingresp_packet{};
        auto ec = ep->async_send(am::packet_variant{p}, as::use_future).get();
        BOOST_TEST(!ec_what_start_with(ec, "packet cannot be send by MQTT protocol"));
    }
    {
        auto p = am::v5::auth_packet{
            am::auth_reason_code::continue_authentication,
            am::properties{}
        };
        auto ec = ep->async_send(am::packet_variant{p}, as::use_future).get();
        BOOST_TEST(!ec_what_start_with(ec, "packet cannot be send by MQTT protocol"));
    }
    {
        auto p = am::v5::disconnect_packet{
            am::disconnect_reason_code::protocol_error,
            am::properties{}
        };
        auto ec = ep->async_send(am::packet_variant{p}, as::use_future).get();
        BOOST_TEST(!ec_what_start_with(ec, "packet cannot be send by MQTT protocol"));
    }

    guard.reset();
    th.join();
}

BOOST_AUTO_TEST_CASE(v5_any) {
    auto version = am::protocol_version::v5;
    as::io_context ioc;
    auto guard = as::make_work_guard(ioc.get_executor());
    std::thread th {
        [&] {
            ioc.run();
        }
    };
    auto ep = am::endpoint<async_mqtt::role::any, async_mqtt::stub_socket>::create(
        version,
        // for stub_socket args
        version,
        ioc.get_executor()
    );

    {
        auto p = am::v5::connect_packet{
            true,   // clean_start
            0x0, // keep_alive
            "cid1",
            std::nullopt,
            "user1",
            "pass1",
            am::properties{}
        };
        auto ec = ep->async_send(am::packet_variant{p}, as::use_future).get();
        BOOST_TEST(!ec_what_start_with(ec, "packet cannot be send by MQTT protocol"));
    }
    {
        auto p = am::v5::connack_packet{
            true,   // session_present
            am::connect_reason_code::not_authorized,
            am::properties{}
        };
        auto ec = ep->async_send(am::packet_variant{p}, as::use_future).get();
        // packet type checking is prior to version checking
        BOOST_TEST(!ec_what_start_with(ec, "packet cannot be send by MQTT protocol"));
    }
    {
        auto p = am::v5::publish_packet{
            "topic1",
            "payload1",
            am::qos::at_most_once | am::pub::retain::yes | am::pub::dup::yes,
            am::properties{}
        };
        auto ec = ep->async_send(am::packet_variant{p}, as::use_future).get();
        BOOST_TEST(!ec_what_start_with(ec, "packet cannot be send by MQTT protocol"));
    }
    {
        auto p = am::v5::puback_packet{
            0x1234, // packet_id
            am::puback_reason_code::packet_identifier_in_use,
            am::properties{}
        };
        auto ec = ep->async_send(am::packet_variant{p}, as::use_future).get();
        BOOST_TEST(!ec_what_start_with(ec, "packet cannot be send by MQTT protocol"));
    }
    {
        auto p = am::v5::pubrec_packet{
            0x1234, // packet_id
            am::pubrec_reason_code::packet_identifier_in_use,
            am::properties{}
        };
        auto ec = ep->async_send(am::packet_variant{p}, as::use_future).get();
        BOOST_TEST(!ec_what_start_with(ec, "packet cannot be send by MQTT protocol"));
    }
    {
        auto p = am::v5::pubrel_packet{
            0x1234, // packet_id
            am::pubrel_reason_code::packet_identifier_not_found,
            am::properties{}
        };
        auto ec = ep->async_send(am::packet_variant{p}, as::use_future).get();
        BOOST_TEST(!ec_what_start_with(ec, "packet cannot be send by MQTT protocol"));
    }
    {
        auto p = am::v5::pubcomp_packet{
            0x1234, // packet_id
            am::pubcomp_reason_code::packet_identifier_not_found,
            am::properties{}
        };
        auto ec = ep->async_send(am::packet_variant{p}, as::use_future).get();
        BOOST_TEST(!ec_what_start_with(ec, "packet cannot be send by MQTT protocol"));
    }
    {
        std::vector<am::topic_subopts> args {
            {"topic1", am::qos::at_most_once | am::sub::nl::yes | am::sub::retain_handling::not_send},
            {"topic2", am::qos::exactly_once | am::sub::rap::retain},
        };
        auto p = am::v5::subscribe_packet{
            0x1234,         // packet_id
            args,
            am::properties{}
        };
        auto ec = ep->async_send(am::packet_variant{p}, as::use_future).get();
        BOOST_TEST(!ec_what_start_with(ec, "packet cannot be send by MQTT protocol"));
    }
    {
        std::vector<am::suback_reason_code> args {
            am::suback_reason_code::granted_qos_1,
            am::suback_reason_code::unspecified_error
        };
        auto p = am::v5::suback_packet{
            0x1234,         // packet_id
            args,
            am::properties{}
        };
        auto ec = ep->async_send(am::packet_variant{p}, as::use_future).get();
        BOOST_TEST(!ec_what_start_with(ec, "packet cannot be send by MQTT protocol"));
    }
    {
        std::vector<am::topic_sharename> args {
            {"topic1"},
            {"topic2"},
        };
        auto p = am::v5::unsubscribe_packet{
            0x1234,         // packet_id
            args,
            am::properties{}
        };
        auto ec = ep->async_send(am::packet_variant{p}, as::use_future).get();
        BOOST_TEST(!ec_what_start_with(ec, "packet cannot be send by MQTT protocol"));
    }
    {
        std::vector<am::unsuback_reason_code> args {
            am::unsuback_reason_code::no_subscription_existed,
            am::unsuback_reason_code::unspecified_error
        };
        auto p = am::v5::unsuback_packet{
            0x1234,         // packet_id
            args,
            am::properties{}
        };
        auto ec = ep->async_send(am::packet_variant{p}, as::use_future).get();
        BOOST_TEST(!ec_what_start_with(ec, "packet cannot be send by MQTT protocol"));
    }
    {
        auto p = am::v5::pingreq_packet{};
        auto ec = ep->async_send(am::packet_variant{p}, as::use_future).get();
        BOOST_TEST(!ec_what_start_with(ec, "packet cannot be send by MQTT protocol"));
    }
    {
        auto p = am::v5::pingresp_packet{};
        auto ec = ep->async_send(am::packet_variant{p}, as::use_future).get();
        BOOST_TEST(!ec_what_start_with(ec, "packet cannot be send by MQTT protocol"));
    }
    {
        auto p = am::v5::auth_packet{
            am::auth_reason_code::continue_authentication,
            am::properties{}
        };
        auto ec = ep->async_send(am::packet_variant{p}, as::use_future).get();
        BOOST_TEST(!ec_what_start_with(ec, "packet cannot be send by MQTT protocol"));
    }
    {
        auto p = am::v5::disconnect_packet{
            am::disconnect_reason_code::protocol_error,
            am::properties{}
        };
        auto ec = ep->async_send(am::packet_variant{p}, as::use_future).get();
        BOOST_TEST(!ec_what_start_with(ec, "packet cannot be send by MQTT protocol"));
    }

    guard.reset();
    th.join();
}

BOOST_AUTO_TEST_SUITE_END()
