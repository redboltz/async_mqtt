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
#include "packet_compare.hpp"

BOOST_AUTO_TEST_SUITE(ut_ep_basic)

namespace am = async_mqtt;
namespace as = boost::asio;

// packet_id is hard coded in this test case for just testing.
// but users need to get packet_id via ep.acquire_unique_packet_id(...)
// see other test cases.

// v3_1_1

BOOST_AUTO_TEST_CASE(valid_client_v3_1_1) {
    auto version = am::protocol_version::v3_1_1;
    as::io_context ioc;

    using ep_t = am::endpoint<async_mqtt::role::client, async_mqtt::stub_socket>;
    ep_t ep{
        version,
        // for stub_socket args
        version,
        ioc
    };

    ep_t ep2{am::force_move(ep)};
}

BOOST_AUTO_TEST_SUITE_END()
