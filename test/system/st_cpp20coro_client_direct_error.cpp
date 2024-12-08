// Copyright Takatoshi Kondo 2024
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "../common/test_main.hpp"
#include "../common/global_fixture.hpp"
#include "broker_runner.hpp"
#include "coro_base.hpp"

#include <async_mqtt/all.hpp>

BOOST_AUTO_TEST_SUITE(st_cpp20coro_client_direct_error)

namespace am = async_mqtt;
namespace as = boost::asio;

using namespace am;

BOOST_AUTO_TEST_CASE(v311) {
    using client = am::client<am::protocol_version::v3_1_1, am::protocol::mqtt>;
    broker_runner br;
    as::io_context ioc;
    auto exe = ioc.get_executor();
    auto amcl = client{exe};
    as::co_spawn(
        exe,
        [&] () -> as::awaitable<void> {
            co_await as::dispatch(
                as::bind_executor(
                    amcl.get_executor(),
                    as::as_tuple(as::use_awaitable)
                )
            );

            // Handshake undlerying layer (Name resolution and TCP handshaking)
            auto [ec_und] = co_await amcl.async_underlying_handshake(
                "127.0.0.1",
                "1883",
                as::as_tuple(as::use_awaitable)
            );
            BOOST_TEST(!ec_und);

            std::string ng_str{static_cast<char>(0b1100'0010u), static_cast<char>(0b1100'0000u)}; // invalid utf8
            // MQTT connect and receive loop start
            auto [ec_con, connack_opt] = co_await amcl.async_start(
                true,   // clean_session
                std::uint16_t(0),      // keep_alive
                ng_str,
                std::nullopt, // will
                "u1",
                "passforu1",
                as::as_tuple(as::use_awaitable)
            );
            BOOST_TEST(ec_con == am::connect_reason_code::client_identifier_not_valid);

            // MQTT send subscribe and wait suback
            std::vector<am::topic_subopts> sub_entry{
                {ng_str, am::qos::at_most_once},
            };
            auto pid_sub_opt = amcl.acquire_unique_packet_id();
            BOOST_CHECK(pid_sub_opt);
            auto [ec_sub, suback_opt] = co_await amcl.async_subscribe(
                *pid_sub_opt,
                am::force_move(sub_entry), // sub_entry variable is required to avoid g++ bug
                as::as_tuple(as::use_awaitable)
            );
            BOOST_TEST(ec_sub == am::disconnect_reason_code::topic_filter_invalid);

            // MQTT send unsubscribe and wait unsuback
            std::vector<am::topic_sharename> unsub_entry{
                {ng_str},
            };
            auto pid_unsub_opt = amcl.acquire_unique_packet_id();
            BOOST_CHECK(pid_unsub_opt);
            auto [ec_unsub, unsuback_opt] = co_await amcl.async_unsubscribe(
                *pid_unsub_opt,
                am::force_move(unsub_entry), // unsub_entry variable is required to avoid g++ bug
                as::as_tuple(as::use_awaitable)
            );
            BOOST_TEST(ec_unsub == am::disconnect_reason_code::topic_filter_invalid);

            auto [ec_pub0, pubres0] = co_await amcl.async_publish(
                ng_str,
                "payload1",
                am::qos::at_most_once,
                as::as_tuple(as::use_awaitable)
            );
            BOOST_TEST(ec_pub0 == am::disconnect_reason_code::topic_name_invalid);
            co_return;
        },
        as::detached
    );
    ioc.run();
}

BOOST_AUTO_TEST_CASE(v5) {
    using client = am::client<am::protocol_version::v5, am::protocol::mqtt>;
    broker_runner br;
    as::io_context ioc;
    auto exe = ioc.get_executor();
    auto amcl = client{exe};
    as::co_spawn(
        exe,
        [&] () -> as::awaitable<void> {
            co_await as::dispatch(
                as::bind_executor(
                    amcl.get_executor(),
                    as::as_tuple(as::use_awaitable)
                )
            );

            // Handshake undlerying layer (Name resolution and TCP handshaking)
            auto [ec_und] = co_await amcl.async_underlying_handshake(
                "127.0.0.1",
                "1883",
                as::as_tuple(as::use_awaitable)
            );
            BOOST_TEST(!ec_und);

            std::string ng_str{static_cast<char>(0b1100'0010u), static_cast<char>(0b1100'0000u)}; // invalid utf8
            // MQTT connect and receive loop start
            auto [ec_con, connack_opt] = co_await amcl.async_start(
                true,   // clean_session
                std::uint16_t(0),      // keep_alive
                ng_str,
                std::nullopt, // will
                "u1",
                "passforu1",
                as::as_tuple(as::use_awaitable)
            );
            BOOST_TEST(ec_con == am::connect_reason_code::client_identifier_not_valid);

            // MQTT send subscribe and wait suback
            std::vector<am::topic_subopts> sub_entry{
                {ng_str, am::qos::at_most_once},
            };
            auto pid_sub_opt = amcl.acquire_unique_packet_id();
            BOOST_CHECK(pid_sub_opt);
            auto [ec_sub, suback_opt] = co_await amcl.async_subscribe(
                *pid_sub_opt,
                am::force_move(sub_entry), // sub_entry variable is required to avoid g++ bug
                as::as_tuple(as::use_awaitable)
            );
            BOOST_TEST(ec_sub == am::disconnect_reason_code::topic_filter_invalid);

            // MQTT send unsubscribe and wait unsuback
            std::vector<am::topic_sharename> unsub_entry{
                {ng_str},
            };
            auto pid_unsub_opt = amcl.acquire_unique_packet_id();
            BOOST_CHECK(pid_unsub_opt);
            auto [ec_unsub, unsuback_opt] = co_await amcl.async_unsubscribe(
                *pid_unsub_opt,
                am::force_move(unsub_entry), // unsub_entry variable is required to avoid g++ bug
                as::as_tuple(as::use_awaitable)
            );
            BOOST_TEST(ec_unsub == am::disconnect_reason_code::topic_filter_invalid);

            auto [ec_pub0, pubres0] = co_await amcl.async_publish(
                ng_str,
                "payload1",
                am::qos::at_most_once,
                as::as_tuple(as::use_awaitable)
            );
            BOOST_TEST(ec_pub0 == am::disconnect_reason_code::topic_name_invalid);

            am::properties props{property::content_type{"test"}}; // invalid property
            auto [ec_disconnect] = co_await amcl.async_disconnect(
                am::disconnect_reason_code::normal_disconnection,
                force_move(props),
                as::as_tuple(as::use_awaitable)
            );
            BOOST_TEST(ec_disconnect == am::disconnect_reason_code::malformed_packet);
            co_return;
        },
        as::detached
    );
    ioc.run();
}

BOOST_AUTO_TEST_SUITE_END()
