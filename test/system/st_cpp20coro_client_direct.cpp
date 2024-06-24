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

BOOST_AUTO_TEST_SUITE(st_cpp20coro_client_direct)

namespace am = async_mqtt;
namespace as = boost::asio;

using namespace am;

BOOST_AUTO_TEST_CASE(v311) {
    broker_runner br;
    as::io_context ioc;
    auto exe = ioc.get_executor();
    auto amcl = am::client<am::protocol_version::v3_1_1, am::protocol::mqtt>::create(exe);
    as::co_spawn(
        exe,
        [&] () -> as::awaitable<void> {
            co_await as::dispatch(
                as::bind_executor(
                    amcl->get_executor(),
                    as::use_awaitable
                )
            );

            // Handshake undlerying layer (Name resolution and TCP handshaking)
            auto [ec_und] = co_await am::async_underlying_handshake(
                amcl->next_layer(),
                "127.0.0.1",
                "1883",
                as::as_tuple(as::use_awaitable)
            );
            BOOST_TEST(!ec_und);

            // MQTT connect and receive loop start
            auto [ec_con, connack_opt] = co_await amcl->async_start(
                true,   // clean_session
                std::uint16_t(0),      // keep_alive
                "cid1",
                std::nullopt, // will
                "u1",
                "passforu1",
                as::as_tuple(as::use_awaitable)
            );
            BOOST_TEST(!ec_con);
            am::packet_variant exp_connack = am::v3_1_1::connack_packet{false, am::connect_return_code::accepted};
            BOOST_CHECK(connack_opt);
            am::packet_variant connack = *connack_opt;
            BOOST_TEST(connack == exp_connack);

            // MQTT send subscribe and wait suback
            std::vector<am::topic_subopts> sub_entry{
                {"topic1", am::qos::at_most_once},
                {"topic2", am::qos::at_least_once},
                {"topic3", am::qos::exactly_once},
            };
            auto pid_sub_opt = amcl->acquire_unique_packet_id();
            BOOST_CHECK(pid_sub_opt);
            auto [ec_sub, suback_opt] = co_await amcl->async_subscribe(
                *pid_sub_opt,
                am::force_move(sub_entry), // sub_entry variable is required to avoid g++ bug
                as::as_tuple(as::use_awaitable)
            );
            BOOST_TEST(!ec_sub);
            am::packet_variant exp_suback =
                am::v3_1_1::suback_packet{
                    *pid_sub_opt,
                    {
                        am::suback_return_code::success_maximum_qos_0,
                        am::suback_return_code::success_maximum_qos_1,
                        am::suback_return_code::success_maximum_qos_2,
                    }
                };
            BOOST_CHECK(suback_opt);
            am::packet_variant suback = *suback_opt;
            BOOST_TEST(suback == exp_suback);

            // MQTT publish QoS0 and wait response (socket write complete)
            auto [ec_pub0, pubres0] = co_await amcl->async_publish(
                "topic1",
                "payload1",
                am::qos::at_most_once,
                as::as_tuple(as::use_awaitable)
            );
            BOOST_TEST(!ec_pub0);
            BOOST_CHECK(!pubres0.puback_opt);
            BOOST_CHECK(!pubres0.pubrec_opt);
            BOOST_CHECK(!pubres0.pubcomp_opt);

            // MQTT publish QoS1 and wait response (puback receive)
            auto [ec_pid1, pid_pub1] = co_await amcl->async_acquire_unique_packet_id(as::as_tuple(as::use_awaitable)); // async version
            BOOST_TEST(!ec_pid1);
            auto [ec_pub1, pubres1] = co_await amcl->async_publish(
                pid_pub1,
                "topic1",
                "payload1",
                am::qos::at_least_once,
                as::as_tuple(as::use_awaitable)
            );
            BOOST_TEST(!ec_pub1);
            am::packet_variant exp_puback1 = am::v3_1_1::puback_packet{pid_pub1};
            BOOST_CHECK(pubres1.puback_opt);
            BOOST_CHECK(!pubres1.pubrec_opt);
            BOOST_CHECK(!pubres1.pubcomp_opt);
            am::packet_variant puback1 = *pubres1.puback_opt;
            BOOST_TEST(puback1 == exp_puback1);

            // MQTT publish QoS2 and wait response (pubrec, pubcomp receive)
            auto pid_pub2 = co_await amcl->async_acquire_unique_packet_id_wait_until(as::use_awaitable); // async version
            auto [ec_pub2, pubres2] = co_await amcl->async_publish(
                pid_pub2,
                "topic1",
                "payload1",
                am::qos::exactly_once,
                as::as_tuple(as::use_awaitable)
            );
            BOOST_TEST(!ec_pub2);
            am::packet_variant exp_pubrec2 = am::v3_1_1::pubrec_packet{pid_pub2};
            am::packet_variant exp_pubcomp2 = am::v3_1_1::pubcomp_packet{pid_pub2};
            BOOST_CHECK(!pubres2.puback_opt);
            BOOST_CHECK(pubres2.pubrec_opt);
            BOOST_CHECK(pubres2.pubcomp_opt);
            am::packet_variant pubrec2 = *pubres2.pubrec_opt;
            BOOST_TEST(pubrec2 == exp_pubrec2);
            am::packet_variant pubcomp2 = *pubres2.pubcomp_opt;
            BOOST_TEST(pubcomp2 == exp_pubcomp2);

            // MQTT send unsubscribe and wait unsuback
            std::vector<am::topic_sharename> unsub_entry{
                {"topic1"},
                {"topic2"},
                {"topic3"},
            };
            auto pid_unsub_opt = amcl->acquire_unique_packet_id();
            BOOST_CHECK(pid_unsub_opt);
            auto [ec_unsub, unsuback_opt] = co_await amcl->async_unsubscribe(
                *pid_unsub_opt,
                am::force_move(unsub_entry), // unsub_entry variable is required to avoid g++ bug
                as::as_tuple(as::use_awaitable)
            );
            BOOST_TEST(!ec_unsub);
            am::packet_variant exp_unsuback =
                am::v3_1_1::unsuback_packet{
                    *pid_unsub_opt,
                };
            BOOST_CHECK(unsuback_opt);
            am::packet_variant unsuback = *unsuback_opt;
            BOOST_TEST(unsuback == exp_unsuback);

            // MQTT disconnect
            auto [ec_disconnect] = co_await amcl->async_disconnect(
                as::as_tuple(as::use_awaitable)
            );
            BOOST_TEST(!ec_disconnect);

            // close
            co_await amcl->async_close(
                as::as_tuple(as::use_awaitable)
            );
            co_return;
        },
        as::detached
    );
    ioc.run();
}

BOOST_AUTO_TEST_CASE(v5) {
    broker_runner br;
    as::io_context ioc;
    auto exe = ioc.get_executor();
    auto amcl = am::client<am::protocol_version::v5, am::protocol::mqtt>::create(exe);
    as::co_spawn(
        exe,
        [&] () -> as::awaitable<void> {
            co_await as::dispatch(
                as::bind_executor(
                    amcl->get_executor(),
                    as::use_awaitable
                )
            );

            // Handshake undlerying layer (Name resolution and TCP handshaking)
            auto [ec_und] = co_await am::async_underlying_handshake(
                amcl->next_layer(),
                "127.0.0.1",
                "1883",
                as::as_tuple(as::use_awaitable)
            );
            BOOST_TEST(!ec_und);

            // MQTT connect and receive loop start
            auto [ec_con, connack_opt] = co_await amcl->async_start(
                true,   // clean_session
                std::uint16_t(0),      // keep_alive
                "cid1",
                std::nullopt, // will
                "u1",
                "passforu1",
                as::as_tuple(as::use_awaitable)
            );
            BOOST_TEST(!ec_con);
            am::packet_variant exp_connack = am::v5::connack_packet{
                false,
                am::connect_reason_code::success,
                am::properties{
                    am::property::topic_alias_maximum{65535},
                    am::property::receive_maximum{65535},
                }
            };
            BOOST_CHECK(connack_opt);
            am::packet_variant connack = *connack_opt;
            BOOST_TEST(connack == exp_connack);

            // MQTT auth
            auto [ec_auth] = co_await amcl->async_auth(
                as::as_tuple(as::use_awaitable)
            );
            BOOST_TEST(!ec_auth);

            // MQTT send subscribe and wait suback
            std::vector<am::topic_subopts> sub_entry{
                {"topic1", am::qos::at_most_once},
                {"topic2", am::qos::at_least_once},
                {"topic3", am::qos::exactly_once},
            };
            auto pid_sub_opt = amcl->acquire_unique_packet_id();
            BOOST_CHECK(pid_sub_opt);
            auto [ec_sub, suback_opt] = co_await amcl->async_subscribe(
                *pid_sub_opt,
                am::force_move(sub_entry), // sub_entry variable is required to avoid g++ bug
                as::as_tuple(as::use_awaitable)
            );
            BOOST_TEST(!ec_sub);
            am::packet_variant exp_suback =
                am::v5::suback_packet{
                    *pid_sub_opt,
                    {
                        am::suback_reason_code::granted_qos_0,
                        am::suback_reason_code::granted_qos_1,
                        am::suback_reason_code::granted_qos_2,
                    }
                };
            BOOST_CHECK(suback_opt);
            am::packet_variant suback = *suback_opt;
            BOOST_TEST(suback == exp_suback);

            // MQTT publish QoS0 and wait response (socket write complete)
            auto [ec_pub0, pubres0] = co_await amcl->async_publish(
                "topic1",
                "payload1",
                am::qos::at_most_once,
                as::as_tuple(as::use_awaitable)
            );
            BOOST_TEST(!ec_pub0);
            BOOST_CHECK(!pubres0.puback_opt);
            BOOST_CHECK(!pubres0.pubrec_opt);
            BOOST_CHECK(!pubres0.pubcomp_opt);

            // MQTT publish QoS1 and wait response (puback receive)
            auto [ec_pid1, pid_pub1] = co_await amcl->async_acquire_unique_packet_id(as::as_tuple(as::use_awaitable)); // async version
            BOOST_TEST(!ec_pid1);
            auto [ec_pub1, pubres1] = co_await amcl->async_publish(
                pid_pub1,
                "topic2",
                "payload2",
                am::qos::at_least_once,
                as::as_tuple(as::use_awaitable)
            );
            BOOST_TEST(!ec_pub1);
            am::packet_variant exp_puback1 = am::v5::puback_packet{pid_pub1};
            BOOST_CHECK(pubres1.puback_opt);
            BOOST_CHECK(!pubres1.pubrec_opt);
            BOOST_CHECK(!pubres1.pubcomp_opt);
            am::packet_variant puback1 = *pubres1.puback_opt;
            BOOST_TEST(puback1 == exp_puback1);

            // MQTT publish QoS2 and wait response (pubrec, pubcomp receive)
            auto pid_pub2 = co_await amcl->async_acquire_unique_packet_id_wait_until(as::use_awaitable); // async version
            auto [ec_pub2, pubres2] = co_await amcl->async_publish(
                pid_pub2,
                "topic3",
                "payload3",
                am::qos::exactly_once,
                as::as_tuple(as::use_awaitable)
            );
            BOOST_TEST(!ec_pub2);
            am::packet_variant exp_pubrec2 = am::v5::pubrec_packet{pid_pub2};
            am::packet_variant exp_pubcomp2 = am::v5::pubcomp_packet{pid_pub2};
            BOOST_CHECK(!pubres2.puback_opt);
            BOOST_CHECK(pubres2.pubrec_opt);
            BOOST_CHECK(pubres2.pubcomp_opt);
            am::packet_variant pubrec2 = *pubres2.pubrec_opt;
            BOOST_TEST(pubrec2 == exp_pubrec2);
            am::packet_variant pubcomp2 = *pubres2.pubcomp_opt;
            BOOST_TEST(pubcomp2 == exp_pubcomp2);

            // MQTT send unsubscribe and wait unsuback
            std::vector<am::topic_sharename> unsub_entry{
                {"topic1"},
                {"topic2"},
                {"topic3"},
            };
            auto pid_unsub_opt = amcl->acquire_unique_packet_id();
            BOOST_CHECK(pid_unsub_opt);
            auto [ec_unsub, unsuback_opt] = co_await amcl->async_unsubscribe(
                *pid_unsub_opt,
                am::force_move(unsub_entry), // unsub_entry variable is required to avoid g++ bug
                as::as_tuple(as::use_awaitable)
            );
            BOOST_TEST(!ec_unsub);
            am::packet_variant exp_unsuback =
                am::v5::unsuback_packet{
                    *pid_unsub_opt,
                    {
                        am::unsuback_reason_code::success,
                        am::unsuback_reason_code::success,
                        am::unsuback_reason_code::success,
                    }
                };
            BOOST_CHECK(unsuback_opt);
            am::packet_variant unsuback = *unsuback_opt;
            BOOST_TEST(unsuback == exp_unsuback);

            // MQTT disconnect
            auto [ec_disconnect] = co_await amcl->async_disconnect(
                as::as_tuple(as::use_awaitable)
            );
            BOOST_TEST(!ec_disconnect);

            // close
            co_await amcl->async_close(
                as::as_tuple(as::use_awaitable)
            );
            co_return;
        },
        as::detached
    );
    ioc.run();
}

BOOST_AUTO_TEST_SUITE_END()
