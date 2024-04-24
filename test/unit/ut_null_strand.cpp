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
#include <async_mqtt/null_strand.hpp>

#include "stub_socket.hpp"

BOOST_AUTO_TEST_SUITE(ut_null_strand)

namespace am = async_mqtt;
namespace as = boost::asio;

// v3_1_1

BOOST_AUTO_TEST_CASE(epst) {
    auto version = am::protocol_version::v3_1_1;
    as::io_context ioc;
    auto guard = as::make_work_guard(ioc.get_executor());
    std::thread th {
        [&] {
            ioc.run();
        }
    };

    auto ep = am::endpoint_st<am::role::client, am::stub_socket>::create(
        version,
        // for stub_socket args
        version,
        ioc
    );
    auto fut1 = ep->acquire_unique_packet_id_wait_until(as::use_future);
    auto pid1 = fut1.get();
    BOOST_TEST(pid1 == 1);

    guard.reset();
    th.join();
}

BOOST_AUTO_TEST_CASE(bep) {
    auto version = am::protocol_version::v3_1_1;
    as::io_context ioc;
    auto guard = as::make_work_guard(ioc.get_executor());
    std::thread th {
        [&] {
            ioc.run();
        }
    };

    auto ep = am::basic_endpoint<am::role::client, 4, am::null_strand, am::stub_socket>::create(
        version,
        // for stub_socket args
        version,
        ioc
    );
    auto fut1 = ep->acquire_unique_packet_id_wait_until(as::use_future);
    auto pid1 = fut1.get();
    BOOST_TEST(pid1 == 1);

    guard.reset();
    th.join();
}

BOOST_AUTO_TEST_SUITE_END()
