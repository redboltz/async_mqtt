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

    auto ep = am::endpoint<am::role::client, am::stub_socket>::create(
        version,
        // for stub_socket args
        version,
        ioc.get_executor()
    );

    for (std::size_t i = 0; i != 0xffff; ++i) {
        ep->async_acquire_unique_packet_id_wait_until(as::use_future).get();
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

    std::promise<void> pro;
    auto fut = pro.get_future();
    std::future<am::packet_id_type> acq_fut1;
    std::future<am::packet_id_type> acq_fut2;
    std::future<am::packet_id_type> acq_fut3;
    std::future<void> rel_fut1;
    std::future<void> rel_fut2;
    std::future<void> rel_fut3;
    as::dispatch(
        as::bind_executor(
            ep->get_executor(),
            [&] {
                acq_fut1 = ep->async_acquire_unique_packet_id_wait_until(as::use_future);
                acq_fut2 = ep->async_acquire_unique_packet_id_wait_until(as::use_future);
                acq_fut3 = ep->async_acquire_unique_packet_id_wait_until(as::use_future);
                rel_fut1 = ep->async_release_packet_id(10001, as::use_future);
                rel_fut2 = ep->async_release_packet_id(10002, as::use_future);
                rel_fut3 = ep->async_release_packet_id(10003, as::use_future);
                pro.set_value();
            }
        )
    );
    fut.get();
    rel_fut1.get();
    rel_fut2.get();
    rel_fut3.get();

    auto pid2 = acq_fut2.get();
    BOOST_TEST(pid2 == 10002);
    auto pid1 = acq_fut1.get();
    BOOST_TEST(pid1 == 10001);
    auto pid3 = acq_fut3.get();
    BOOST_TEST(pid3 == 10003);
    guard.reset();
    th.join();
}

BOOST_AUTO_TEST_SUITE_END()
