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

BOOST_AUTO_TEST_SUITE(ut_ep_pid)

namespace am = async_mqtt;
using namespace am::literals;
namespace as = boost::asio;

// v3_1_1

BOOST_AUTO_TEST_CASE(wait_until) {
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

    for (std::size_t i = 0; i != 0xffff; ++i) {
        ep->acquire_unique_packet_id_wait_until(as::use_future).get();
    }

    as::dispatch(
        as::bind_executor(
            ep->get_executor(),
            [&] {
                auto pid_opt = ep->acquire_unique_packet_id();
                BOOST_CHECK(!pid_opt);
            }
        )
    );

    auto fut1 = ep->acquire_unique_packet_id_wait_until(as::use_future);
    auto fut2 = ep->acquire_unique_packet_id_wait_until(as::use_future);
    auto fut3 = ep->acquire_unique_packet_id_wait_until(as::use_future);
    ep->release_packet_id(10001, as::use_future).get();
    ep->release_packet_id(10002, as::use_future).get();
    ep->release_packet_id(10003, as::use_future).get();
    auto pid2 = fut2.get();
    BOOST_TEST(pid2 == 10002);
    auto pid1 = fut1.get();
    BOOST_TEST(pid1 == 10001);
    auto pid3 = fut3.get();
    BOOST_TEST(pid3 == 10003);
    guard.reset();
    th.join();
}

BOOST_AUTO_TEST_SUITE_END()
