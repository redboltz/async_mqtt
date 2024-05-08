// Copyright Takatoshi Kondo 2024
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "../common/test_main.hpp"
#include "../common/global_fixture.hpp"

#include <thread>

#include <boost/asio.hpp>

#include <async_mqtt/endpoint.hpp>
#include <async_mqtt/util/packet_variant_operator.hpp>

#include "cpp20coro_stub_socket.hpp"

BOOST_AUTO_TEST_SUITE(ut_broker)

namespace am = async_mqtt;
using namespace am::literals;
namespace as = boost::asio;

BOOST_AUTO_TEST_CASE(pingresp_tout_v311) {
    auto version = am::protocol_version::v3_1_1;
    as::io_context ioc;
    as::co_spawn(
        ioc.get_executor(),
        [&]() -> as::awaitable<void> {
            auto ep = am::endpoint<async_mqtt::role::client, am::cpp20coro_stub_socket>::create(
                version,
                // for stub_socket args
                version,
                ioc.get_executor()
            );
            ep->set_pingresp_recv_timeout_ms(0); // for coverage
            ep->set_pingresp_recv_timeout_ms(10);
            // prepare connect
            {
                auto connect = am::v3_1_1::connect_packet{
                    true,   // clean_session
                    0x1234, // keep_alive
                    "cid1"_mb
                };
                auto se = co_await ep->send(connect, as::deferred);
                BOOST_TEST(!se);
                co_await ep->next_layer().wait_response(as::deferred);

                auto connack = am::v3_1_1::connack_packet{
                    false,   // session_present
                    am::connect_return_code::accepted
                };
                co_await ep->next_layer().emulate_recv(connack, as::deferred);
                co_await ep->recv(as::deferred);
            }
            // test scenario
            {
                auto se = co_await ep->send(am::v3_1_1::pingreq_packet{}, as::deferred);
                BOOST_TEST(!se);
                co_await ep->next_layer().wait_response(as::deferred);

                as::steady_timer tim{ioc.get_executor(), std::chrono::milliseconds(20)};
                co_await tim.async_wait(as::deferred);
                auto close = co_await ep->next_layer().wait_response(as::deferred);
                BOOST_TEST(am::is_close(close));
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
            auto ep = am::endpoint<async_mqtt::role::client, am::cpp20coro_stub_socket>::create(
                version,
                // for stub_socket args
                version,
                ioc.get_executor()
            );
            ep->set_pingresp_recv_timeout_ms(10);
            // prepare connect
            {
                auto connect = am::v5::connect_packet{
                    true,   // clean_session
                    0x1234, // keep_alive
                    "cid1"_mb
                };
                auto se = co_await ep->send(connect, as::deferred);
                BOOST_TEST(!se);
                co_await ep->next_layer().wait_response(as::deferred);

                auto connack = am::v5::connack_packet{
                    false,   // session_present
                    am::connect_reason_code::success
                };
                co_await ep->next_layer().emulate_recv(connack, as::deferred);
                co_await ep->recv(as::deferred);
            }
            // test scenario
            {
                auto se = co_await ep->send(am::v5::pingreq_packet{}, as::deferred);
                BOOST_TEST(!se);
                co_await ep->next_layer().wait_response(as::deferred);

                as::steady_timer tim{ioc.get_executor(), std::chrono::milliseconds(20)};
                co_await tim.async_wait(as::deferred);
                auto exp = am::v5::disconnect_packet{
                    am::disconnect_reason_code::keep_alive_timeout,
                    am::properties{}
                };
                auto disconnect = co_await ep->next_layer().wait_response(as::deferred);
                BOOST_TEST(disconnect == exp);
                auto close = co_await ep->next_layer().wait_response(as::deferred);
                BOOST_TEST(am::is_close(close));
            }

            co_return;
        },
        as::detached
    );
    ioc.run();
}


BOOST_AUTO_TEST_SUITE_END()
