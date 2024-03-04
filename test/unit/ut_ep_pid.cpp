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
        ioc
    );

    for (std::size_t i = 0; i != 0xffff; ++i) {
        ep->acquire_unique_packet_id_wait_until(as::use_future).get();
    }

    auto fut = ep->acquire_unique_packet_id_wait_until(as::use_future);
    ep->release_packet_id(12345, as::use_future).get();
    auto pid = fut.get();
    BOOST_TEST(pid == 12345);
    guard.reset();
    th.join();
}

BOOST_AUTO_TEST_SUITE_END()
