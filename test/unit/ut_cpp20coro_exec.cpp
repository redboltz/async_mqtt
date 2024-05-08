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

#include "stub_socket.hpp"
#include <async_mqtt/util/packet_variant_operator.hpp>

BOOST_AUTO_TEST_SUITE(ut_cpp20coro_exec)

namespace am = async_mqtt;
using namespace am::literals;
namespace as = boost::asio;

// coroutine executor and endpoint executor are different.
// After co_await, the current executor should be back to coroutine executor.
BOOST_AUTO_TEST_CASE(different) {
    auto version = am::protocol_version::v3_1_1;
    as::io_context ioc_coro;
    as::io_context ioc_ep;
    auto str_coro = as::make_strand(ioc_coro.get_executor());
    auto str_ep = as::make_strand(ioc_ep.get_executor());

    auto guard_coro = as::make_work_guard(ioc_coro.get_executor());
    auto guard_ep = as::make_work_guard(ioc_ep.get_executor());

    std::thread th_coro {
        [&] {
            ioc_coro.run();
        }
    };
    std::thread th_ep {
        [&] {
            ioc_ep.run();
        }
    };

    auto ep = am::endpoint<async_mqtt::role::client, async_mqtt::stub_socket>::create(
        version,
        // for stub_socket args
        version,
        str_ep
    );
    as::co_spawn(
        str_coro,
        [&]() -> as::awaitable<void> {
            BOOST_TEST(str_coro.running_in_this_thread());

            auto connect = am::v3_1_1::connect_packet{
                true,   // clean_session
                0, // keep_alive
                "cid1"_mb,
                am::nullopt, // will
                "user1"_mb,
                "pass1"_mb
            };
            auto connack = am::v3_1_1::connack_packet{
                true,   // session_present
                am::connect_return_code::accepted
            };

            auto se = co_await ep->send(connect, as::deferred);
            BOOST_TEST(str_coro.running_in_this_thread());
            ep->next_layer().set_recv_packets(
                {
                    // receive packets
                    connack,
                }
            );

            auto pv = co_await ep->recv(as::deferred);
            BOOST_TEST(str_coro.running_in_this_thread());

            auto pid_opt = co_await ep->acquire_unique_packet_id(as::deferred);
            BOOST_TEST(str_coro.running_in_this_thread());

            co_await ep->register_packet_id(*pid_opt, as::deferred);
            BOOST_TEST(str_coro.running_in_this_thread());

            co_await ep->release_packet_id(*pid_opt, as::deferred);
            BOOST_TEST(str_coro.running_in_this_thread());

            auto packets = co_await ep->get_stored_packets(as::deferred);
            BOOST_TEST(str_coro.running_in_this_thread());

            co_await ep->restore_packets(packets, as::deferred);
            BOOST_TEST(str_coro.running_in_this_thread());

            auto pub = am::v5::publish_packet{
                "topic1"_mb,
                "payload1"_mb,
                am::qos::at_most_once | am::pub::retain::yes | am::pub::dup::yes
            };

            co_await ep->regulate_for_store(pub, as::deferred);
            BOOST_TEST(str_coro.running_in_this_thread());

            co_await ep->close(as::deferred);
            BOOST_TEST(str_coro.running_in_this_thread());

            guard_coro.reset();
            co_return;
        },
        as::detached
    );

    th_coro.join();
    guard_ep.reset();
    th_ep.join();

}

// coroutine executor and endpoint executor are different.
// After co_await, the current executor should be bound executor.
BOOST_AUTO_TEST_CASE(bind) {
    auto version = am::protocol_version::v3_1_1;
    as::io_context ioc_coro;
    as::io_context ioc_ep;
    auto str_coro = as::make_strand(ioc_coro.get_executor());
    auto str_ep = as::make_strand(ioc_ep.get_executor());

    auto guard_coro = as::make_work_guard(ioc_coro.get_executor());
    auto guard_ep = as::make_work_guard(ioc_ep.get_executor());

    std::thread th_coro {
        [&] {
            ioc_coro.run();
        }
    };
    std::thread th_ep {
        [&] {
            ioc_ep.run();
        }
    };

    auto ep = am::endpoint<async_mqtt::role::client, async_mqtt::stub_socket>::create(
        version,
        // for stub_socket args
        version,
        str_ep
    );
    as::co_spawn(
        str_coro,
        [&]() -> as::awaitable<void> {
            BOOST_TEST(str_coro.running_in_this_thread());

            auto connect = am::v3_1_1::connect_packet{
                true,   // clean_session
                0, // keep_alive
                "cid1"_mb,
                am::nullopt, // will
                "user1"_mb,
                "pass1"_mb
            };
            auto connack = am::v3_1_1::connack_packet{
                true,   // session_present
                am::connect_return_code::accepted
            };

            auto se = co_await ep->send(connect, as::bind_executor(str_ep, as::deferred));
            BOOST_TEST(str_ep.running_in_this_thread());
            ep->next_layer().set_recv_packets(
                {
                    // receive packets
                    connack,
                }
            );

            auto pv = co_await ep->recv(as::bind_executor(str_ep, as::deferred));
            BOOST_TEST(str_ep.running_in_this_thread());

            auto pid_opt = co_await ep->acquire_unique_packet_id(as::bind_executor(str_ep, as::deferred));
            BOOST_TEST(str_ep.running_in_this_thread());

            co_await ep->register_packet_id(*pid_opt, as::bind_executor(str_ep, as::deferred));
            BOOST_TEST(str_ep.running_in_this_thread());

            co_await ep->release_packet_id(*pid_opt, as::bind_executor(str_ep, as::deferred));
            BOOST_TEST(str_ep.running_in_this_thread());

            auto packets = co_await ep->get_stored_packets(as::bind_executor(str_ep, as::deferred));
            BOOST_TEST(str_ep.running_in_this_thread());

            co_await ep->restore_packets(packets, as::bind_executor(str_ep, as::deferred));
            BOOST_TEST(str_ep.running_in_this_thread());

            auto pub = am::v5::publish_packet{
                "topic1"_mb,
                "payload1"_mb,
                am::qos::at_most_once | am::pub::retain::yes | am::pub::dup::yes
            };

            co_await ep->regulate_for_store(pub, as::bind_executor(str_ep, as::deferred));
            BOOST_TEST(str_ep.running_in_this_thread());

            co_await ep->close(as::bind_executor(str_ep, as::deferred));
            BOOST_TEST(str_ep.running_in_this_thread());

            guard_coro.reset();
            co_return;
        },
        as::detached
    );

    th_coro.join();
    guard_ep.reset();
    th_ep.join();

}

// coroutine executor and endpoint executor are the same.
// After co_await, both given strand and endpoint's internal strand
// are satisfied. (running on the strands)
// This is recommended usage.
BOOST_AUTO_TEST_CASE(same) {
    auto version = am::protocol_version::v3_1_1;
    as::io_context ioc;
    auto str = as::make_strand(ioc.get_executor());

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
        str
    );
    as::co_spawn(
        str,
        [&]() -> as::awaitable<void> {
            BOOST_TEST(str.running_in_this_thread());
            BOOST_TEST(!ep->in_strand());

            auto connect = am::v3_1_1::connect_packet{
                true,   // clean_session
                0, // keep_alive
                "cid1"_mb,
                am::nullopt, // will
                "user1"_mb,
                "pass1"_mb
            };
            auto connack = am::v3_1_1::connack_packet{
                true,   // session_present
                am::connect_return_code::accepted
            };

            auto se = co_await ep->send(connect, as::deferred);
            BOOST_TEST(str.running_in_this_thread());
            BOOST_TEST(ep->in_strand());
            ep->next_layer().set_recv_packets(
                {
                    // receive packets
                    connack,
                }
            );

            auto pv = co_await ep->recv(as::deferred);
            BOOST_TEST(str.running_in_this_thread());
            BOOST_TEST(ep->in_strand());

            auto pid_opt = co_await ep->acquire_unique_packet_id(as::deferred);
            BOOST_TEST(str.running_in_this_thread());
            BOOST_TEST(ep->in_strand());

            co_await ep->register_packet_id(*pid_opt, as::deferred);
            BOOST_TEST(str.running_in_this_thread());
            BOOST_TEST(ep->in_strand());

            co_await ep->release_packet_id(*pid_opt, as::deferred);
            BOOST_TEST(str.running_in_this_thread());
            BOOST_TEST(ep->in_strand());

            auto packets = co_await ep->get_stored_packets(as::deferred);
            BOOST_TEST(str.running_in_this_thread());
            BOOST_TEST(ep->in_strand());

            co_await ep->restore_packets(packets, as::deferred);
            BOOST_TEST(str.running_in_this_thread());
            BOOST_TEST(ep->in_strand());

            auto pub = am::v5::publish_packet{
                "topic1"_mb,
                "payload1"_mb,
                am::qos::at_most_once | am::pub::retain::yes | am::pub::dup::yes
            };

            co_await ep->regulate_for_store(pub, as::deferred);
            BOOST_TEST(str.running_in_this_thread());
            BOOST_TEST(ep->in_strand());

            co_await ep->close(as::deferred);
            BOOST_TEST(str.running_in_this_thread());
            BOOST_TEST(ep->in_strand());

            guard.reset();
            co_return;
        },
        as::detached
    );

    th.join();
}

BOOST_AUTO_TEST_SUITE_END()
