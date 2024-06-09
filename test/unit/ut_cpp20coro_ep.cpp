// Copyright Takatoshi Kondo 2024
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "../common/test_main.hpp"
#include "../common/global_fixture.hpp"

#include <thread>

#include <boost/asio.hpp>
#include <boost/asio/experimental/awaitable_operators.hpp>

#include <async_mqtt/endpoint.hpp>

#include "cpp20coro_stub_socket.hpp"

BOOST_AUTO_TEST_SUITE(ut_cpp20coro_ep)

namespace am = async_mqtt;
namespace as = boost::asio;
using namespace as::experimental::awaitable_operators;

using namespace std::literals::string_view_literals;

// pingresp

BOOST_AUTO_TEST_CASE(pingresp_v311) {
    auto version = am::protocol_version::v3_1_1;
    as::io_context ioc;
    as::co_spawn(
        ioc.get_executor(),
        [&]() -> as::awaitable<void> {
            auto exe = co_await as::this_coro::executor;
            auto ep = am::endpoint<async_mqtt::role::server, am::cpp20coro_stub_socket>::create(
                version,
                // for stub_socket args
                version,
                am::force_move(exe)
            );
            ep->set_auto_ping_response(true);
            // prepare connect
            {
                auto connect = am::v3_1_1::connect_packet{
                    true,   // clean_session
                    0x1, // keep_alive
                    "cid1"
                };
                co_await ep->next_layer().emulate_recv(connect, as::as_tuple(as::deferred));
                co_await ep->async_recv(as::as_tuple(as::deferred));

                auto connack = am::v3_1_1::connack_packet{
                    false,   // session_present
                    am::connect_return_code::accepted
                };
                auto [ec] = co_await ep->async_send(connack, as::as_tuple(as::deferred));
                BOOST_TEST(!ec);
                co_await ep->next_layer().wait_response(as::as_tuple(as::deferred));
            }
            // test scenario
            {
                co_await ep->next_layer().emulate_recv(am::v3_1_1::pingreq_packet{}, as::as_tuple(as::deferred));
                co_await ep->async_recv(as::as_tuple(as::deferred));
                auto [ec, pingresp] = co_await ep->next_layer().wait_response(as::as_tuple(as::deferred));
                BOOST_TEST(pingresp == am::v3_1_1::pingresp_packet{});
                co_await ep->async_close(as::deferred);
                co_await ep->next_layer().wait_response(as::as_tuple(as::deferred));
            }
            co_return;
        },
        as::detached
    );
    ioc.run();
}

BOOST_AUTO_TEST_CASE(pingresp_v5) {
    auto version = am::protocol_version::v5;
    as::io_context ioc;
    as::co_spawn(
        ioc.get_executor(),
        [&]() -> as::awaitable<void> {
            auto exe = co_await as::this_coro::executor;
            auto ep = am::endpoint<async_mqtt::role::server, am::cpp20coro_stub_socket>::create(
                version,
                // for stub_socket args
                version,
                am::force_move(exe)
            );
            ep->set_auto_ping_response(true);
            // prepare connect
            {
                auto connect = am::v5::connect_packet{
                    true,   // clean_session
                    0x1, // keep_alive
                    "cid1"
                };
                co_await ep->next_layer().emulate_recv(connect, as::as_tuple(as::deferred));
                co_await ep->async_recv(as::as_tuple(as::deferred));

                auto connack = am::v5::connack_packet{
                    false,   // session_present
                    am::connect_reason_code::success
                };
                auto [ec] = co_await ep->async_send(connack, as::as_tuple(as::deferred));
                BOOST_TEST(!ec);
                co_await ep->next_layer().wait_response(as::as_tuple(as::deferred));
            }
            // test scenario
            {
                co_await ep->next_layer().emulate_recv(am::v5::pingreq_packet{}, as::as_tuple(as::deferred));
                co_await ep->async_recv(as::as_tuple(as::deferred));
                auto [ec, pingresp] = co_await ep->next_layer().wait_response(as::as_tuple(as::deferred));
                BOOST_TEST(pingresp == am::v5::pingresp_packet{});
                co_await ep->async_close(as::deferred);
                co_await ep->next_layer().wait_response(as::as_tuple(as::deferred));
            }
            co_return;
        },
        as::detached
    );
    ioc.run();
}

BOOST_AUTO_TEST_CASE(pingresp_no_tout_v311) {
    auto version = am::protocol_version::v3_1_1;
    as::io_context ioc;
    as::co_spawn(
        ioc.get_executor(),
        [&]() -> as::awaitable<void> {
            auto exe = co_await as::this_coro::executor;
            auto ep = am::endpoint<async_mqtt::role::client, am::cpp20coro_stub_socket>::create(
                version,
                // for stub_socket args
                version,
                am::force_move(exe)
            );
            ep->set_pingresp_recv_timeout_ms(10);
            // prepare connect
            {
                auto connect = am::v3_1_1::connect_packet{
                    true,   // clean_session
                    0x1234, // keep_alive
                    "cid1"
                };
                auto [ec] = co_await ep->async_send(connect, as::as_tuple(as::deferred));
                BOOST_TEST(!ec);
                co_await ep->next_layer().wait_response(as::as_tuple(as::deferred));

                auto connack = am::v3_1_1::connack_packet{
                    false,   // session_present
                    am::connect_return_code::accepted
                };
                co_await ep->next_layer().emulate_recv(connack, as::as_tuple(as::deferred));
                co_await ep->async_recv(as::as_tuple(as::deferred));
            }
            // test scenario
            {
                auto [ec] = co_await ep->async_send(am::v3_1_1::pingreq_packet{}, as::as_tuple(as::deferred));
                BOOST_TEST(!ec);
                co_await ep->next_layer().wait_response(as::as_tuple(as::deferred));

                co_await ep->next_layer().emulate_recv(am::v3_1_1::pingresp_packet{}, as::as_tuple(as::deferred));
                co_await ep->async_recv(as::as_tuple(as::deferred));

                as::steady_timer tim{ioc.get_executor(), std::chrono::milliseconds(20)};
                co_await tim.async_wait(as::as_tuple(as::deferred));

                co_await ep->async_close(as::deferred);
                co_await ep->next_layer().wait_response(as::as_tuple(as::deferred));
            }

            co_return;
        },
        as::detached
    );
    ioc.run();
}

BOOST_AUTO_TEST_CASE(pingresp_no_tout_v5) {
    auto version = am::protocol_version::v5;
    as::io_context ioc;
    as::co_spawn(
        ioc.get_executor(),
        [&]() -> as::awaitable<void> {
            auto exe = co_await as::this_coro::executor;
            auto ep = am::endpoint<async_mqtt::role::client, am::cpp20coro_stub_socket>::create(
                version,
                // for stub_socket args
                version,
                am::force_move(exe)
            );
            ep->set_pingresp_recv_timeout_ms(10);
            // prepare connect
            {
                auto connect = am::v5::connect_packet{
                    true,   // clean_session
                    0x1234, // keep_alive
                    "cid1"
                };
                auto [ec] = co_await ep->async_send(connect, as::as_tuple(as::deferred));
                BOOST_TEST(!ec);
                co_await ep->next_layer().wait_response(as::as_tuple(as::deferred));

                auto connack = am::v5::connack_packet{
                    false,   // session_present
                    am::connect_reason_code::success
                };
                co_await ep->next_layer().emulate_recv(connack, as::as_tuple(as::deferred));
                co_await ep->async_recv(as::as_tuple(as::deferred));
            }
            // test scenario
            {
                auto [ec] = co_await ep->async_send(am::v5::pingreq_packet{}, as::as_tuple(as::deferred));
                BOOST_TEST(!ec);
                co_await ep->next_layer().wait_response(as::as_tuple(as::deferred));

                co_await ep->next_layer().emulate_recv(am::v5::pingresp_packet{}, as::as_tuple(as::deferred));
                co_await ep->async_recv(as::as_tuple(as::deferred));

                as::steady_timer tim{ioc.get_executor(), std::chrono::milliseconds(20)};
                co_await tim.async_wait(as::as_tuple(as::deferred));

                co_await ep->async_close(as::deferred);
                co_await ep->next_layer().wait_response(as::as_tuple(as::deferred));
            }

            co_return;
        },
        as::detached
    );
    ioc.run();
}

BOOST_AUTO_TEST_CASE(pingresp_tout_v311) {
    auto version = am::protocol_version::v3_1_1;
    as::io_context ioc;
    as::co_spawn(
        ioc.get_executor(),
        [&]() -> as::awaitable<void> {
            auto exe = co_await as::this_coro::executor;
            auto ep = am::endpoint<async_mqtt::role::client, am::cpp20coro_stub_socket>::create(
                version,
                // for stub_socket args
                version,
                am::force_move(exe)
            );
            ep->set_pingreq_send_interval_ms(0); // for coverage
            ep->set_pingresp_recv_timeout_ms(0); // for coverage
            ep->set_pingresp_recv_timeout_ms(10);
            // prepare connect
            {
                auto connect = am::v3_1_1::connect_packet{
                    true,   // clean_session
                    0x1234, // keep_alive
                    "cid1"
                };
                auto [ec] = co_await ep->async_send(connect, as::as_tuple(as::deferred));
                BOOST_TEST(!ec);
                co_await ep->next_layer().wait_response(as::as_tuple(as::deferred));

                auto connack = am::v3_1_1::connack_packet{
                    false,   // session_present
                    am::connect_return_code::accepted
                };
                co_await ep->next_layer().emulate_recv(connack, as::as_tuple(as::deferred));
                co_await ep->async_recv(as::as_tuple(as::deferred));
            }
            // test scenario
            {
                auto [ec] = co_await ep->async_send(am::v3_1_1::pingreq_packet{}, as::as_tuple(as::deferred));
                BOOST_TEST(!ec);
                co_await ep->next_layer().wait_response(as::as_tuple(as::deferred));

                as::steady_timer tim{ioc.get_executor(), std::chrono::milliseconds(20)};
                co_await tim.async_wait(as::as_tuple(as::deferred));
                {
                    auto [ec, close] = co_await ep->next_layer().wait_response(as::as_tuple(as::deferred));
                    BOOST_TEST(ec == am::errc::connection_reset);
                }
            }

            co_return;
        },
        as::detached
    );
    ioc.run();
}

BOOST_AUTO_TEST_CASE(pingresp_tout_v5) {
    auto version = am::protocol_version::v5;
    as::io_context ioc;
    as::co_spawn(
        ioc.get_executor(),
        [&]() -> as::awaitable<void> {
            auto exe = co_await as::this_coro::executor;
            auto ep = am::endpoint<async_mqtt::role::client, am::cpp20coro_stub_socket>::create(
                version,
                // for stub_socket args
                version,
                am::force_move(exe)
            );
            ep->set_pingresp_recv_timeout_ms(10);
            // prepare connect
            {
                auto connect = am::v5::connect_packet{
                    true,   // clean_session
                    0x1234, // keep_alive
                    "cid1"
                };
                auto [ec] = co_await ep->async_send(connect, as::as_tuple(as::deferred));
                BOOST_TEST(!ec);
                co_await ep->next_layer().wait_response(as::as_tuple(as::deferred));

                auto connack = am::v5::connack_packet{
                    false,   // session_present
                    am::connect_reason_code::success
                };
                co_await ep->next_layer().emulate_recv(connack, as::as_tuple(as::deferred));
                co_await ep->async_recv(as::as_tuple(as::deferred));
            }
            // test scenario
            {
                auto [ec] = co_await ep->async_send(am::v5::pingreq_packet{}, as::as_tuple(as::deferred));
                BOOST_TEST(!ec);
                co_await ep->next_layer().wait_response(as::as_tuple(as::deferred));

                as::steady_timer tim{ioc.get_executor(), std::chrono::milliseconds(20)};
                co_await tim.async_wait(as::as_tuple(as::deferred));
                auto exp = am::v5::disconnect_packet{
                    am::disconnect_reason_code::keep_alive_timeout,
                    am::properties{}
                };
                {
                    auto [ec, disconnect] = co_await ep->next_layer().wait_response(as::as_tuple(as::deferred));
                    BOOST_TEST(!ec);
                    BOOST_TEST(disconnect == exp);
                }
                {
                    auto [ec, close] = co_await ep->next_layer().wait_response(as::as_tuple(as::deferred));
                    BOOST_TEST(ec == am::errc::connection_reset);
                }
            }

            co_return;
        },
        as::detached
    );
    ioc.run();
}


BOOST_AUTO_TEST_CASE(bulk_write) {
    auto version = am::protocol_version::v3_1_1;
    as::io_context ioc;
    as::co_spawn(
        ioc.get_executor(),
        [&]() -> as::awaitable<void> {
            auto exe = co_await as::this_coro::executor;
            auto ep = am::endpoint<async_mqtt::role::client, am::cpp20coro_stub_socket>::create(
                version,
                // for stub_socket args
                version,
                am::force_move(exe)
            );
            ep->set_bulk_write(true);
            // prepare connect
            {
                auto connect = am::v3_1_1::connect_packet{
                    true,   // clean_session
                    0x1234, // keep_alive
                    "cid1"
                };
                auto [ec] = co_await ep->async_send(connect, as::as_tuple(as::deferred));
                BOOST_TEST(!ec);
                co_await ep->next_layer().wait_response(as::as_tuple(as::deferred));

                auto connack = am::v3_1_1::connack_packet{
                    false,   // session_present
                    am::connect_return_code::accepted
                };
                co_await ep->next_layer().emulate_recv(connack, as::as_tuple(as::deferred));
                co_await ep->async_recv(as::as_tuple(as::deferred));
            }
            // test scenario
            {
                auto [ec1, ec2t] =
                    co_await (
                        ep->async_send(am::v3_1_1::pingreq_packet{}, as::as_tuple(as::use_awaitable)) &&
                        ep->async_send(am::v3_1_1::pingreq_packet{}, as::as_tuple(as::use_awaitable))
                    );
                auto [ec2] = ec2t;
                BOOST_TEST(!ec1);
                BOOST_TEST(!ec2);

                co_await ep->async_close(as::deferred);
                co_await ep->next_layer().wait_response(as::as_tuple(as::deferred));
            }

            co_return;
        },
        as::detached
    );
    ioc.run();
}

// async_recv remaining length error

BOOST_AUTO_TEST_CASE(remaining_length_error) {
    auto version = am::protocol_version::v3_1_1;
    as::io_context ioc;
    as::co_spawn(
        ioc.get_executor(),
        [&]() -> as::awaitable<void> {
            auto exe = co_await as::this_coro::executor;
            auto ep = am::endpoint<async_mqtt::role::server, am::cpp20coro_stub_socket>::create(
                version,
                // for stub_socket args
                version,
                am::force_move(exe)
            );
            // test scenario
            {
                co_await ep->next_layer().emulate_recv(
                    "\xe0\x80\x80\x80\x80\x00"sv, // invalid remaining length
                    as::as_tuple(as::deferred)
                );
                auto [ec, _] = co_await ep->async_recv(as::as_tuple(as::deferred));
                BOOST_TEST(ec == am::disconnect_reason_code::packet_too_large);
            }
            co_return;
        },
        as::detached
    );
    ioc.run();
}

// async_send completion error

BOOST_AUTO_TEST_CASE(sub_send_error_v5) {
    auto version = am::protocol_version::v5;
    as::io_context ioc;
    as::co_spawn(
        ioc.get_executor(),
        [&]() -> as::awaitable<void> {
            auto exe = co_await as::this_coro::executor;
            auto ep = am::endpoint<async_mqtt::role::client, am::cpp20coro_stub_socket>::create(
                version,
                // for stub_socket args
                version,
                am::force_move(exe)
            );
            // prepare connect
            {
                auto connect = am::v5::connect_packet{
                    true,   // clean_session
                    0x1234, // keep_alive
                    "cid1"
                };
                auto [ec] = co_await ep->async_send(connect, as::as_tuple(as::deferred));
                BOOST_TEST(!ec);
                co_await ep->next_layer().wait_response(as::as_tuple(as::deferred));

                auto connack = am::v5::connack_packet{
                    false,   // session_present
                    am::connect_reason_code::success
                };
                co_await ep->next_layer().emulate_recv(connack, as::as_tuple(as::deferred));
                co_await ep->async_recv(as::as_tuple(as::deferred));
            }
            // test scenario
            {
                ep->next_layer().set_send_error_code(
                    make_error_code(as::error::operation_aborted)
                );
                std::vector<am::topic_subopts> entries {
                    {"topic1", am::qos::at_most_once},
                };
                auto pid = *ep->acquire_unique_packet_id();
                auto [ec] = co_await ep->async_send(
                    am::v5::subscribe_packet{
                        pid,
                        entries
                    },
                    as::as_tuple(as::use_awaitable)
                );
                BOOST_TEST(ec == as::error::operation_aborted);
                // check pid is released
                BOOST_TEST(ep->register_packet_id(pid));
                co_await ep->async_close(as::deferred);
                co_await ep->next_layer().wait_response(as::as_tuple(as::deferred));
            }

            co_return;
        },
        as::detached
    );
    ioc.run();
}

BOOST_AUTO_TEST_CASE(unsub_send_error_v5) {
    auto version = am::protocol_version::v5;
    as::io_context ioc;
    as::co_spawn(
        ioc.get_executor(),
        [&]() -> as::awaitable<void> {
            auto exe = co_await as::this_coro::executor;
            auto ep = am::endpoint<async_mqtt::role::client, am::cpp20coro_stub_socket>::create(
                version,
                // for stub_socket args
                version,
                am::force_move(exe)
            );
            // prepare connect
            {
                auto connect = am::v5::connect_packet{
                    true,   // clean_session
                    0x1234, // keep_alive
                    "cid1"
                };
                auto [ec] = co_await ep->async_send(connect, as::as_tuple(as::deferred));
                BOOST_TEST(!ec);
                co_await ep->next_layer().wait_response(as::as_tuple(as::deferred));

                auto connack = am::v5::connack_packet{
                    false,   // session_present
                    am::connect_reason_code::success
                };
                co_await ep->next_layer().emulate_recv(connack, as::as_tuple(as::deferred));
                co_await ep->async_recv(as::as_tuple(as::deferred));
            }
            // test scenario
            {
                ep->next_layer().set_send_error_code(
                    make_error_code(as::error::operation_aborted)
                );
                std::vector<am::topic_sharename> entries {
                    {"topic1"},
                };
                auto pid = *ep->acquire_unique_packet_id();
                auto [ec] = co_await ep->async_send(
                    am::v5::unsubscribe_packet{
                        pid,
                        entries
                    },
                    as::as_tuple(as::use_awaitable)
                );
                BOOST_TEST(ec == as::error::operation_aborted);
                // check pid is released
                BOOST_TEST(ep->register_packet_id(pid));
                co_await ep->async_close(as::deferred);
                co_await ep->next_layer().wait_response(as::as_tuple(as::deferred));
            }

            co_return;
        },
        as::detached
    );
    ioc.run();
}

BOOST_AUTO_TEST_CASE(pub_send_error_v5) {
    auto version = am::protocol_version::v5;
    as::io_context ioc;
    as::co_spawn(
        ioc.get_executor(),
        [&]() -> as::awaitable<void> {
            auto exe = co_await as::this_coro::executor;
            auto ep = am::endpoint<async_mqtt::role::client, am::cpp20coro_stub_socket>::create(
                version,
                // for stub_socket args
                version,
                am::force_move(exe)
            );
            // prepare connect
            {
                auto connect = am::v5::connect_packet{
                    true,   // clean_session
                    0x1234, // keep_alive
                    "cid1"
                };
                auto [ec] = co_await ep->async_send(connect, as::as_tuple(as::deferred));
                BOOST_TEST(!ec);
                co_await ep->next_layer().wait_response(as::as_tuple(as::deferred));

                auto connack = am::v5::connack_packet{
                    false,   // session_present
                    am::connect_reason_code::success
                };
                co_await ep->next_layer().emulate_recv(connack, as::as_tuple(as::deferred));
                co_await ep->async_recv(as::as_tuple(as::deferred));
            }
            // test scenario
            {
                ep->next_layer().set_send_error_code(
                    make_error_code(as::error::operation_aborted)
                );
                auto pid = *ep->acquire_unique_packet_id();
                auto [ec] = co_await ep->async_send(
                    am::v5::publish_packet{
                        pid,
                        "topic1",
                        "payload1",
                        am::qos::at_least_once,
                        am::properties{}
                    },
                    as::as_tuple(as::use_awaitable)
                );
                BOOST_TEST(ec == as::error::operation_aborted);
                // check pid is released
                BOOST_TEST(ep->register_packet_id(pid));
                co_await ep->async_close(as::deferred);
                co_await ep->next_layer().wait_response(as::as_tuple(as::deferred));
            }

            co_return;
        },
        as::detached
    );
    ioc.run();
}

// topic_alias_invalid

BOOST_AUTO_TEST_CASE(pub_recv_non_exist_ta_v5) {
    auto version = am::protocol_version::v5;
    as::io_context ioc;
    as::co_spawn(
        ioc.get_executor(),
        [&]() -> as::awaitable<void> {
            auto exe = co_await as::this_coro::executor;
            auto ep = am::endpoint<async_mqtt::role::client, am::cpp20coro_stub_socket>::create(
                version,
                // for stub_socket args
                version,
                am::force_move(exe)
            );
            // prepare connect
            {
                auto connect = am::v5::connect_packet{
                    true,   // clean_session
                    0x1234, // keep_alive
                    "cid1",
                    std::nullopt,
                    std::nullopt,
                    am::properties{
                        am::property::topic_alias_maximum{0xffff},
                        am::property::session_expiry_interval{am::session_never_expire}
                    }
                };
                auto [ec] = co_await ep->async_send(connect, as::as_tuple(as::deferred));
                BOOST_TEST(!ec);
                co_await ep->next_layer().wait_response(as::as_tuple(as::deferred));

                auto connack = am::v5::connack_packet{
                    false,   // session_present
                    am::connect_reason_code::success
                };
                co_await ep->next_layer().emulate_recv(connack, as::as_tuple(as::deferred));
                co_await ep->async_recv(as::as_tuple(as::deferred));
            }
            // test scenario
            {
                auto pid = *ep->acquire_unique_packet_id();
                auto publish = am::v5::publish_packet{
                    pid,
                    "",
                    "payload1",
                    am::qos::at_least_once,
                    am::properties{
                        am::property::topic_alias{1}
                    }
                };

                auto exp = am::v5::disconnect_packet{
                    am::disconnect_reason_code::topic_alias_invalid
                };
                co_await ep->next_layer().emulate_recv(publish, as::as_tuple(as::deferred));
                auto [ec_recv, pv] = co_await ep->async_recv(as::as_tuple(as::deferred));
                BOOST_TEST(ec_recv == am::disconnect_reason_code::topic_alias_invalid);
                BOOST_CHECK(!pv);

                {
                    auto [ec, disconnect] = co_await ep->next_layer().wait_response(as::as_tuple(as::deferred));
                    BOOST_TEST(ec == am::error_code{});
                    BOOST_TEST(disconnect == exp);
                }
                {
                    auto [ec, _] = co_await ep->next_layer().wait_response(as::as_tuple(as::deferred));
                    BOOST_TEST(ec == am::errc::connection_reset);
                }
            }

            co_return;
        },
        as::detached
    );
    ioc.run();
}

BOOST_AUTO_TEST_CASE(pub_recv_no_ta_v5) {
    auto version = am::protocol_version::v5;
    as::io_context ioc;
    as::co_spawn(
        ioc.get_executor(),
        [&]() -> as::awaitable<void> {
            auto exe = co_await as::this_coro::executor;
            auto ep = am::endpoint<async_mqtt::role::client, am::cpp20coro_stub_socket>::create(
                version,
                // for stub_socket args
                version,
                am::force_move(exe)
            );
            // prepare connect
            {
                auto connect = am::v5::connect_packet{
                    true,   // clean_session
                    0x1234, // keep_alive
                    "cid1",
                    std::nullopt,
                    std::nullopt,
                    am::properties{
                        am::property::topic_alias_maximum{0xffff},
                        am::property::session_expiry_interval{am::session_never_expire}
                    }
                };
                auto [ec] = co_await ep->async_send(connect, as::as_tuple(as::deferred));
                BOOST_TEST(!ec);
                co_await ep->next_layer().wait_response(as::as_tuple(as::deferred));

                auto connack = am::v5::connack_packet{
                    false,   // session_present
                    am::connect_reason_code::success,
                };
                co_await ep->next_layer().emulate_recv(connack, as::as_tuple(as::deferred));
                co_await ep->async_recv(as::as_tuple(as::deferred));
            }
            // test scenario
            {
                auto pid = *ep->acquire_unique_packet_id();
                auto publish = am::v5::publish_packet{
                    pid,
                    "",
                    "payload1",
                    am::qos::at_least_once
                };

                auto exp = am::v5::disconnect_packet{
                    am::disconnect_reason_code::topic_alias_invalid
                };
                co_await ep->next_layer().emulate_recv(publish, as::as_tuple(as::deferred));
                auto [ec_recv, pv] = co_await ep->async_recv(as::as_tuple(as::deferred));
                BOOST_TEST(ec_recv == am::disconnect_reason_code::topic_alias_invalid);
                BOOST_CHECK(!pv);

                {
                    auto [ec, disconnect] = co_await ep->next_layer().wait_response(as::as_tuple(as::deferred));
                    BOOST_TEST(ec == am::error_code{});
                    BOOST_TEST(disconnect == exp);
                }
                {
                    auto [ec, _] = co_await ep->next_layer().wait_response(as::as_tuple(as::deferred));
                    BOOST_TEST(ec == am::errc::connection_reset);
                }
            }

            co_return;
        },
        as::detached
    );
    ioc.run();
}

BOOST_AUTO_TEST_CASE(pub_recv_max_zero_ta_v5) {
    auto version = am::protocol_version::v5;
    as::io_context ioc;
    as::co_spawn(
        ioc.get_executor(),
        [&]() -> as::awaitable<void> {
            auto exe = co_await as::this_coro::executor;
            auto ep = am::endpoint<async_mqtt::role::client, am::cpp20coro_stub_socket>::create(
                version,
                // for stub_socket args
                version,
                am::force_move(exe)
            );
            // prepare connect
            {
                auto connect = am::v5::connect_packet{
                    true,   // clean_session
                    0x1234, // keep_alive
                    "cid1",
                    std::nullopt,
                    std::nullopt,
                    am::properties{
                        am::property::session_expiry_interval{am::session_never_expire}
                    }
                };
                auto [ec] = co_await ep->async_send(connect, as::as_tuple(as::deferred));
                BOOST_TEST(!ec);
                co_await ep->next_layer().wait_response(as::as_tuple(as::deferred));

                auto connack = am::v5::connack_packet{
                    false,   // session_present
                    am::connect_reason_code::success
                };
                co_await ep->next_layer().emulate_recv(connack, as::as_tuple(as::deferred));
                co_await ep->async_recv(as::as_tuple(as::deferred));
            }
            // test scenario
            {
                auto pid = *ep->acquire_unique_packet_id();
                auto publish = am::v5::publish_packet{
                    pid,
                    "",
                    "payload1",
                    am::qos::at_least_once,
                    am::properties{
                        am::property::topic_alias{1}
                    }
                };

                auto exp = am::v5::disconnect_packet{
                    am::disconnect_reason_code::topic_alias_invalid
                };
                co_await ep->next_layer().emulate_recv(publish, as::as_tuple(as::deferred));
                auto [ec_recv, pv] = co_await ep->async_recv(as::as_tuple(as::deferred));
                BOOST_TEST(ec_recv == am::disconnect_reason_code::topic_alias_invalid);
                BOOST_CHECK(!pv);

                {
                    auto [ec, disconnect] = co_await ep->next_layer().wait_response(as::as_tuple(as::deferred));
                    BOOST_TEST(ec == am::error_code{});
                    BOOST_TEST(disconnect == exp);
                }
                {
                    auto [ec, _] = co_await ep->next_layer().wait_response(as::as_tuple(as::deferred));
                    BOOST_TEST(ec == am::errc::connection_reset);
                }
            }

            co_return;
        },
        as::detached
    );
    ioc.run();
}

BOOST_AUTO_TEST_CASE(pub_send_max_zero_ta_v5) {
    auto version = am::protocol_version::v5;
    as::io_context ioc;
    as::co_spawn(
        ioc.get_executor(),
        [&]() -> as::awaitable<void> {
            auto exe = co_await as::this_coro::executor;
            auto ep = am::endpoint<async_mqtt::role::client, am::cpp20coro_stub_socket>::create(
                version,
                // for stub_socket args
                version,
                am::force_move(exe)
            );
            // prepare connect
            {
                auto connect = am::v5::connect_packet{
                    true,   // clean_session
                    0x1234, // keep_alive
                    "cid1",
                    std::nullopt,
                    std::nullopt,
                    am::properties{
                        am::property::session_expiry_interval{am::session_never_expire}
                    }
                };
                auto [ec] = co_await ep->async_send(connect, as::as_tuple(as::deferred));
                BOOST_TEST(!ec);
                co_await ep->next_layer().wait_response(as::as_tuple(as::deferred));

                auto connack = am::v5::connack_packet{
                    false,   // session_present
                    am::connect_reason_code::success
                };
                co_await ep->next_layer().emulate_recv(connack, as::as_tuple(as::deferred));
                co_await ep->async_recv(as::as_tuple(as::deferred));
            }
            // test scenario
            {
                auto pid = *ep->acquire_unique_packet_id();
                auto publish = am::v5::publish_packet{
                    pid,
                    "",
                    "payload1",
                    am::qos::at_least_once,
                    am::properties{
                        am::property::topic_alias{1}
                    }
                };

                auto [ec] = co_await ep->async_send(publish, as::as_tuple(as::deferred));
                BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);

                co_await ep->async_close(as::deferred);
                co_await ep->next_layer().wait_response(as::as_tuple(as::deferred));
            }

            co_return;
        },
        as::detached
    );
    ioc.run();
}

BOOST_AUTO_TEST_CASE(pub_send_size_over_store_ta_v5) {
    auto version = am::protocol_version::v5;
    as::io_context ioc;
    as::co_spawn(
        ioc.get_executor(),
        [&]() -> as::awaitable<void> {
            auto exe = co_await as::this_coro::executor;
            auto ep = am::endpoint<async_mqtt::role::client, am::cpp20coro_stub_socket>::create(
                version,
                // for stub_socket args
                version,
                am::force_move(exe)
            );
            // prepare connect
            {
                auto connect = am::v5::connect_packet{
                    true,   // clean_session
                    0x1234, // keep_alive
                    "cid1",
                    std::nullopt,
                    std::nullopt,
                    am::properties{
                        am::property::session_expiry_interval{am::session_never_expire}
                    }
                };
                auto [ec] = co_await ep->async_send(connect, as::as_tuple(as::deferred));
                BOOST_TEST(!ec);
                co_await ep->next_layer().wait_response(as::as_tuple(as::deferred));

                auto connack = am::v5::connack_packet{
                    false,   // session_present
                    am::connect_reason_code::success,
                    am::properties{
                        am::property::topic_alias_maximum{0xffff},
                        am::property::maximum_packet_size{41}
                    }
                };
                co_await ep->next_layer().emulate_recv(connack, as::as_tuple(as::deferred));
                co_await ep->async_recv(as::as_tuple(as::deferred));
            }
            // prepare register ta
            {
                auto publish = am::v5::publish_packet{
                    "topic12345678901234567890",
                    "payload1",
                    am::qos::at_most_once,
                    am::properties{
                        am::property::topic_alias{1}
                    }
                };
                auto [ec] = co_await ep->async_send(publish, as::as_tuple(as::deferred));
                BOOST_TEST(!ec);
                co_await ep->next_layer().wait_response(as::as_tuple(as::deferred));
            }
            // test scenario use ta but size over for store
            {
                auto pid = *ep->acquire_unique_packet_id();
                auto publish = am::v5::publish_packet{
                    pid,
                    "",
                    "payload1234",
                    am::qos::at_least_once,
                    am::properties{
                        am::property::topic_alias{1}
                    }
                };

                auto [ec] = co_await ep->async_send(publish, as::as_tuple(as::deferred));
                BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);

                co_await ep->async_close(as::deferred);
                co_await ep->next_layer().wait_response(as::as_tuple(as::deferred));
            }

            co_return;
        },
        as::detached
    );
    ioc.run();
}

// pub response invalid

BOOST_AUTO_TEST_CASE(puback_recv_no_pid_v311) {
    auto version = am::protocol_version::v3_1_1;
    as::io_context ioc;
    as::co_spawn(
        ioc.get_executor(),
        [&]() -> as::awaitable<void> {
            auto exe = co_await as::this_coro::executor;
            auto ep = am::endpoint<async_mqtt::role::client, am::cpp20coro_stub_socket>::create(
                version,
                // for stub_socket args
                version,
                am::force_move(exe)
            );
            // prepare connect
            {
                auto connect = am::v3_1_1::connect_packet{
                    true,   // clean_session
                    0x1234, // keep_alive
                    "cid1"
                };
                auto [ec] = co_await ep->async_send(connect, as::as_tuple(as::deferred));
                BOOST_TEST(!ec);
                co_await ep->next_layer().wait_response(as::as_tuple(as::deferred));

                auto connack = am::v3_1_1::connack_packet{
                    false,   // session_present
                    am::connect_return_code::accepted
                };
                co_await ep->next_layer().emulate_recv(connack, as::as_tuple(as::deferred));
                co_await ep->async_recv(as::as_tuple(as::deferred));
            }
            // test scenario
            {
                auto publish = am::v3_1_1::puback_packet{
                    1 // packet_id
                };

                co_await ep->next_layer().emulate_recv(publish, as::as_tuple(as::deferred));
                auto [ec_recv, pv] = co_await ep->async_recv(as::as_tuple(as::deferred));
                BOOST_TEST(ec_recv == am::disconnect_reason_code::protocol_error);
                BOOST_CHECK(!pv);

                auto [ec, disconnect] = co_await ep->next_layer().wait_response(as::as_tuple(as::deferred));
                BOOST_TEST(ec == am::errc::connection_reset);
            }

            co_return;
        },
        as::detached
    );
    ioc.run();
}

BOOST_AUTO_TEST_CASE(puback_recv_no_pid_v5) {
    auto version = am::protocol_version::v5;
    as::io_context ioc;
    as::co_spawn(
        ioc.get_executor(),
        [&]() -> as::awaitable<void> {
            auto exe = co_await as::this_coro::executor;
            auto ep = am::endpoint<async_mqtt::role::client, am::cpp20coro_stub_socket>::create(
                version,
                // for stub_socket args
                version,
                am::force_move(exe)
            );
            // prepare connect
            {
                auto connect = am::v5::connect_packet{
                    true,   // clean_session
                    0x1234, // keep_alive
                    "cid1"
                };
                auto [ec] = co_await ep->async_send(connect, as::as_tuple(as::deferred));
                BOOST_TEST(!ec);
                co_await ep->next_layer().wait_response(as::as_tuple(as::deferred));

                auto connack = am::v5::connack_packet{
                    false,   // session_present
                    am::connect_reason_code::success
                };
                co_await ep->next_layer().emulate_recv(connack, as::as_tuple(as::deferred));
                co_await ep->async_recv(as::as_tuple(as::deferred));
            }
            // test scenario
            {
                auto publish = am::v5::puback_packet{
                    1 // packet_id
                };

                auto exp = am::v5::disconnect_packet{
                    am::disconnect_reason_code::protocol_error
                };
                co_await ep->next_layer().emulate_recv(publish, as::as_tuple(as::deferred));
                auto [ec_recv, pv] = co_await ep->async_recv(as::as_tuple(as::deferred));
                BOOST_TEST(ec_recv == am::disconnect_reason_code::protocol_error);
                BOOST_CHECK(!pv);

                {
                    auto [ec, disconnect] = co_await ep->next_layer().wait_response(as::as_tuple(as::deferred));
                    BOOST_TEST(ec == am::error_code{});
                    BOOST_TEST(disconnect == exp);
                }
                {
                    auto [ec, _] = co_await ep->next_layer().wait_response(as::as_tuple(as::deferred));
                    BOOST_TEST(ec == am::errc::connection_reset);
                }
            }

            co_return;
        },
        as::detached
    );
    ioc.run();
}

BOOST_AUTO_TEST_CASE(pubrec_recv_no_pid_v311) {
    auto version = am::protocol_version::v3_1_1;
    as::io_context ioc;
    as::co_spawn(
        ioc.get_executor(),
        [&]() -> as::awaitable<void> {
            auto exe = co_await as::this_coro::executor;
            auto ep = am::endpoint<async_mqtt::role::client, am::cpp20coro_stub_socket>::create(
                version,
                // for stub_socket args
                version,
                am::force_move(exe)
            );
            // prepare connect
            {
                auto connect = am::v3_1_1::connect_packet{
                    true,   // clean_session
                    0x1234, // keep_alive
                    "cid1"
                };
                auto [ec] = co_await ep->async_send(connect, as::as_tuple(as::deferred));
                BOOST_TEST(!ec);
                co_await ep->next_layer().wait_response(as::as_tuple(as::deferred));

                auto connack = am::v3_1_1::connack_packet{
                    false,   // session_present
                    am::connect_return_code::accepted
                };
                co_await ep->next_layer().emulate_recv(connack, as::as_tuple(as::deferred));
                co_await ep->async_recv(as::as_tuple(as::deferred));
            }
            // test scenario
            {
                auto publish = am::v3_1_1::pubrec_packet{
                    1 // packet_id
                };

                co_await ep->next_layer().emulate_recv(publish, as::as_tuple(as::deferred));
                auto [ec_recv, pv] = co_await ep->async_recv(as::as_tuple(as::deferred));
                BOOST_TEST(ec_recv == am::disconnect_reason_code::protocol_error);
                BOOST_CHECK(!pv);

                auto [ec, disconnect] = co_await ep->next_layer().wait_response(as::as_tuple(as::deferred));
                BOOST_TEST(ec == am::errc::connection_reset);
            }

            co_return;
        },
        as::detached
    );
    ioc.run();
}

BOOST_AUTO_TEST_CASE(pubrec_recv_no_pid_v5) {
    auto version = am::protocol_version::v5;
    as::io_context ioc;
    as::co_spawn(
        ioc.get_executor(),
        [&]() -> as::awaitable<void> {
            auto exe = co_await as::this_coro::executor;
            auto ep = am::endpoint<async_mqtt::role::client, am::cpp20coro_stub_socket>::create(
                version,
                // for stub_socket args
                version,
                am::force_move(exe)
            );
            // prepare connect
            {
                auto connect = am::v5::connect_packet{
                    true,   // clean_session
                    0x1234, // keep_alive
                    "cid1"
                };
                auto [ec] = co_await ep->async_send(connect, as::as_tuple(as::deferred));
                BOOST_TEST(!ec);
                co_await ep->next_layer().wait_response(as::as_tuple(as::deferred));

                auto connack = am::v5::connack_packet{
                    false,   // session_present
                    am::connect_reason_code::success
                };
                co_await ep->next_layer().emulate_recv(connack, as::as_tuple(as::deferred));
                co_await ep->async_recv(as::as_tuple(as::deferred));
            }
            // test scenario
            {
                auto publish = am::v5::pubrec_packet{
                    1 // packet_id
                };

                auto exp = am::v5::disconnect_packet{
                    am::disconnect_reason_code::protocol_error
                };
                co_await ep->next_layer().emulate_recv(publish, as::as_tuple(as::deferred));
                auto [ec_recv, pv] = co_await ep->async_recv(as::as_tuple(as::deferred));
                BOOST_TEST(ec_recv == am::disconnect_reason_code::protocol_error);
                BOOST_CHECK(!pv);

                {
                    auto [ec, disconnect] = co_await ep->next_layer().wait_response(as::as_tuple(as::deferred));
                    BOOST_TEST(ec == am::error_code{});
                    BOOST_TEST(disconnect == exp);
                }
                {
                    auto [ec, _] = co_await ep->next_layer().wait_response(as::as_tuple(as::deferred));
                    BOOST_TEST(ec == am::errc::connection_reset);
                }
            }

            co_return;
        },
        as::detached
    );
    ioc.run();
}

BOOST_AUTO_TEST_CASE(pubcomp_recv_no_pid_v311) {
    auto version = am::protocol_version::v3_1_1;
    as::io_context ioc;
    as::co_spawn(
        ioc.get_executor(),
        [&]() -> as::awaitable<void> {
            auto exe = co_await as::this_coro::executor;
            auto ep = am::endpoint<async_mqtt::role::client, am::cpp20coro_stub_socket>::create(
                version,
                // for stub_socket args
                version,
                am::force_move(exe)
            );
            // prepare connect
            {
                auto connect = am::v3_1_1::connect_packet{
                    true,   // clean_session
                    0x1234, // keep_alive
                    "cid1"
                };
                auto [ec] = co_await ep->async_send(connect, as::as_tuple(as::deferred));
                BOOST_TEST(!ec);
                co_await ep->next_layer().wait_response(as::as_tuple(as::deferred));

                auto connack = am::v3_1_1::connack_packet{
                    false,   // session_present
                    am::connect_return_code::accepted
                };
                co_await ep->next_layer().emulate_recv(connack, as::as_tuple(as::deferred));
                co_await ep->async_recv(as::as_tuple(as::deferred));
            }
            // test scenario
            {
                auto publish = am::v3_1_1::pubcomp_packet{
                    1 // packet_id
                };

                co_await ep->next_layer().emulate_recv(publish, as::as_tuple(as::deferred));
                auto [ec_recv, pv] = co_await ep->async_recv(as::as_tuple(as::deferred));
                BOOST_TEST(ec_recv == am::disconnect_reason_code::protocol_error);
                BOOST_CHECK(!pv);

                auto [ec, disconnect] = co_await ep->next_layer().wait_response(as::as_tuple(as::deferred));
                BOOST_TEST(ec == am::errc::connection_reset);
            }

            co_return;
        },
        as::detached
    );
    ioc.run();
}

BOOST_AUTO_TEST_CASE(pubcomp_recv_no_pid_v5) {
    auto version = am::protocol_version::v5;
    as::io_context ioc;
    as::co_spawn(
        ioc.get_executor(),
        [&]() -> as::awaitable<void> {
            auto exe = co_await as::this_coro::executor;
            auto ep = am::endpoint<async_mqtt::role::client, am::cpp20coro_stub_socket>::create(
                version,
                // for stub_socket args
                version,
                am::force_move(exe)
            );
            // prepare connect
            {
                auto connect = am::v5::connect_packet{
                    true,   // clean_session
                    0x1234, // keep_alive
                    "cid1"
                };
                auto [ec] = co_await ep->async_send(connect, as::as_tuple(as::deferred));
                BOOST_TEST(!ec);
                co_await ep->next_layer().wait_response(as::as_tuple(as::deferred));

                auto connack = am::v5::connack_packet{
                    false,   // session_present
                    am::connect_reason_code::success
                };
                co_await ep->next_layer().emulate_recv(connack, as::as_tuple(as::deferred));
                co_await ep->async_recv(as::as_tuple(as::deferred));
            }
            // test scenario
            {
                auto publish = am::v5::pubcomp_packet{
                    1 // packet_id
                };

                auto exp = am::v5::disconnect_packet{
                    am::disconnect_reason_code::protocol_error
                };
                co_await ep->next_layer().emulate_recv(publish, as::as_tuple(as::deferred));
                auto [ec_recv, pv] = co_await ep->async_recv(as::as_tuple(as::deferred));
                BOOST_TEST(ec_recv == am::disconnect_reason_code::protocol_error);
                BOOST_CHECK(!pv);

                {
                    auto [ec, disconnect] = co_await ep->next_layer().wait_response(as::as_tuple(as::deferred));
                    BOOST_TEST(ec == am::error_code{});
                    BOOST_TEST(disconnect == exp);
                }
                {
                    auto [ec, _] = co_await ep->next_layer().wait_response(as::as_tuple(as::deferred));
                    BOOST_TEST(ec == am::errc::connection_reset);
                }
            }

            co_return;
        },
        as::detached
    );
    ioc.run();
}

// close

BOOST_AUTO_TEST_CASE(close_close) {
    auto version = am::protocol_version::v5;
    as::io_context ioc;
    as::co_spawn(
        ioc.get_executor(),
        [&]() -> as::awaitable<void> {
            auto exe = co_await as::this_coro::executor;
            auto ep = am::endpoint<async_mqtt::role::client, am::cpp20coro_stub_socket>::create(
                version,
                // for stub_socket args
                version,
                am::force_move(exe)
            );
            // prepare connect
            {
                auto connect = am::v5::connect_packet{
                    true,   // clean_session
                    0x1234, // keep_alive
                    "cid1"
                };
                auto [ec] = co_await ep->async_send(connect, as::as_tuple(as::deferred));
                BOOST_TEST(!ec);
                co_await ep->next_layer().wait_response(as::as_tuple(as::deferred));

                auto connack = am::v5::connack_packet{
                    false,   // session_present
                    am::connect_reason_code::success
                };
                co_await ep->next_layer().emulate_recv(connack, as::as_tuple(as::deferred));
                co_await ep->async_recv(as::as_tuple(as::deferred));
            }
            // test scenario
            {
                co_await (
                    ep->async_close(as::use_awaitable) &&
                    ep->async_close(as::use_awaitable)
                );
                co_await ep->next_layer().wait_response(as::as_tuple(as::deferred));
            }

            co_return;
        },
        as::detached
    );
    ioc.run();
}

// invalid connect

BOOST_AUTO_TEST_CASE(connect_bad_version_v5) {
    auto version = am::protocol_version::v5;
    as::io_context ioc;
    as::co_spawn(
        ioc.get_executor(),
        [&]() -> as::awaitable<void> {
            auto exe = co_await as::this_coro::executor;
            auto ep = am::endpoint<async_mqtt::role::client, am::cpp20coro_stub_socket>::create(
                version,
                // for stub_socket args
                version,
                am::force_move(exe)
            );
            // prepare connect
            {
                auto connect = am::v3_1_1::connect_packet{
                    true,   // clean_session
                    0x1234, // keep_alive
                    "cid1"
                };
                {
                    auto [ec] = co_await ep->async_send(connect, as::as_tuple(as::deferred));
                    BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
                }
            }

            co_return;
        },
        as::detached
    );
    ioc.run();
}

BOOST_AUTO_TEST_CASE(connect_bad_timing_v5) {
    auto version = am::protocol_version::v5;
    as::io_context ioc;
    as::co_spawn(
        ioc.get_executor(),
        [&]() -> as::awaitable<void> {
            auto exe = co_await as::this_coro::executor;
            auto ep = am::endpoint<async_mqtt::role::client, am::cpp20coro_stub_socket>::create(
                version,
                // for stub_socket args
                version,
                am::force_move(exe)
            );
            // prepare connect
            {
                auto connect = am::v5::connect_packet{
                    true,   // clean_session
                    0x1234, // keep_alive
                    "cid1"
                };
                {
                    auto [ec] = co_await ep->async_send(connect, as::as_tuple(as::deferred));
                    BOOST_TEST(!ec);
                    co_await ep->next_layer().wait_response(as::as_tuple(as::deferred));
                }
                {
                    auto [ec] = co_await ep->async_send(connect, as::as_tuple(as::deferred));
                    BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
                }
            }

            co_return;
        },
        as::detached
    );
    ioc.run();
}

// invalid connack

BOOST_AUTO_TEST_CASE(connack_bad_version_v5) {
    auto version = am::protocol_version::v5;
    as::io_context ioc;
    as::co_spawn(
        ioc.get_executor(),
        [&]() -> as::awaitable<void> {
            auto exe = co_await as::this_coro::executor;
            auto ep = am::endpoint<async_mqtt::role::server, am::cpp20coro_stub_socket>::create(
                version,
                // for stub_socket args
                version,
                am::force_move(exe)
            );
            {
                auto connect = am::v5::connect_packet{
                    true,   // clean_start
                    0, // keep_alive
                    "cid1"
                };
                co_await ep->next_layer().emulate_recv(connect, as::as_tuple(as::deferred));
                co_await ep->async_recv(as::as_tuple(as::deferred));
            }
            // prepare connack
            {
                auto connack = am::v3_1_1::connack_packet{true, am::connect_return_code::accepted};
                {
                    auto [ec] = co_await ep->async_send(connack, as::as_tuple(as::deferred));
                    BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
                }
            }

            co_return;
        },
        as::detached
    );
    ioc.run();
}

BOOST_AUTO_TEST_CASE(connack_bad_timing_v5) {
    auto version = am::protocol_version::v5;
    as::io_context ioc;
    as::co_spawn(
        ioc.get_executor(),
        [&]() -> as::awaitable<void> {
            auto exe = co_await as::this_coro::executor;
            auto ep = am::endpoint<async_mqtt::role::server, am::cpp20coro_stub_socket>::create(
                version,
                // for stub_socket args
                version,
                am::force_move(exe)
            );
            // prepare connack
            {
                auto connack = am::v5::connack_packet{true, am::connect_reason_code::success};
                auto [ec] = co_await ep->async_send(connack, as::as_tuple(as::deferred));
                BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
            }

            co_return;
        },
        as::detached
    );
    ioc.run();
}

// invalid auth

BOOST_AUTO_TEST_CASE(auth_bad_version) {
    auto version = am::protocol_version::v3_1_1;
    as::io_context ioc;
    as::co_spawn(
        ioc.get_executor(),
        [&]() -> as::awaitable<void> {
            auto exe = co_await as::this_coro::executor;
            auto ep = am::endpoint<async_mqtt::role::server, am::cpp20coro_stub_socket>::create(
                version,
                // for stub_socket args
                version,
                am::force_move(exe)
            );
            {
                auto connect = am::v3_1_1::connect_packet{
                    true,   // clean_session
                    0, // keep_alive
                    "cid1"
                };
                co_await ep->next_layer().emulate_recv(connect, as::as_tuple(as::deferred));
                co_await ep->async_recv(as::as_tuple(as::deferred));
            }
            // prepare auth
            {
                auto auth = am::v5::auth_packet{};
                {
                    auto [ec] = co_await ep->async_send(auth, as::as_tuple(as::deferred));
                    BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
                }
            }

            co_return;
        },
        as::detached
    );
    ioc.run();
}

// invalid version after connected (general packets)

BOOST_AUTO_TEST_CASE(general_bad_version) {
    auto version = am::protocol_version::v3_1_1;
    as::io_context ioc;
    as::co_spawn(
        ioc.get_executor(),
        [&]() -> as::awaitable<void> {
            auto exe = co_await as::this_coro::executor;
            auto ep = am::endpoint<async_mqtt::role::client, am::cpp20coro_stub_socket>::create(
                version,
                // for stub_socket args
                version,
                am::force_move(exe)
            );
            // prepare connect
            {
                auto connect = am::v3_1_1::connect_packet{
                    true,   // clean_session
                    0, // keep_alive
                    "cid1"
                };
                auto [ec] = co_await ep->async_send(connect, as::as_tuple(as::deferred));
                BOOST_TEST(!ec);
                co_await ep->next_layer().wait_response(as::as_tuple(as::deferred));

                auto connack = am::v3_1_1::connack_packet{
                    false,   // session_present
                    am::connect_return_code::accepted
                };
                co_await ep->next_layer().emulate_recv(connack, as::as_tuple(as::deferred));
                co_await ep->async_recv(as::as_tuple(as::deferred));
            }
            // test scenario
            {
                auto [ec] = co_await ep->async_send(
                    am::v5::publish_packet{
                        "topic1",
                        "payload1",
                        am::qos::at_most_once,
                        am::properties{}
                    },
                    as::as_tuple(as::use_awaitable)
                );
                BOOST_TEST(ec == am::mqtt_error::packet_not_allowed_to_send);
                co_await ep->async_close(as::deferred);
                co_await ep->next_layer().wait_response(as::as_tuple(as::deferred));
            }

            co_return;
        },
        as::detached
    );
    ioc.run();
}

// send pubrec as error

BOOST_AUTO_TEST_CASE(pubrec_as_error_v5) {
    auto version = am::protocol_version::v5;
    as::io_context ioc;
    as::co_spawn(
        ioc.get_executor(),
        [&]() -> as::awaitable<void> {
            auto exe = co_await as::this_coro::executor;
            auto ep = am::endpoint<async_mqtt::role::client, am::cpp20coro_stub_socket>::create(
                version,
                // for stub_socket args
                version,
                am::force_move(exe)
            );
            // prepare connect
            {
                auto connect = am::v5::connect_packet{
                    true,   // clean_start
                    0, // keep_alive
                    "cid1"
                };
                auto [ec] = co_await ep->async_send(connect, as::as_tuple(as::deferred));
                BOOST_TEST(!ec);
                co_await ep->next_layer().wait_response(as::as_tuple(as::deferred));

                auto connack = am::v5::connack_packet{
                    false,   // session_present
                    am::connect_reason_code::success
                };
                co_await ep->next_layer().emulate_recv(connack, as::as_tuple(as::deferred));
                co_await ep->async_recv(as::as_tuple(as::deferred));
            }
            // test scenario
            {
                auto publish =
                    am::v5::publish_packet{
                        1,
                        "topic1",
                        "payload1",
                        am::qos::exactly_once
                    };
                co_await ep->next_layer().emulate_recv(publish, as::as_tuple(as::deferred));
                co_await ep->async_recv(as::as_tuple(as::deferred));
                auto [ec] = co_await ep->async_send(
                    am::v5::pubrec_packet{
                        1,
                        am::pubrec_reason_code::unspecified_error
                    },
                    as::as_tuple(as::use_awaitable)
                );
                BOOST_TEST(!ec);
                co_await ep->async_close(as::deferred);
                co_await ep->next_layer().wait_response(as::as_tuple(as::deferred));
            }

            co_return;
        },
        as::detached
    );
    ioc.run();
}



BOOST_AUTO_TEST_SUITE_END()
