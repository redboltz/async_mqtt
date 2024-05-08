// Copyright Takatoshi Kondo 2023
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "../common/test_main.hpp"
#include "../common/global_fixture.hpp"

#include <boost/asio.hpp>

#include <async_mqtt/endpoint.hpp>
#include "stub_socket.hpp"

namespace am = async_mqtt;
using namespace am::literals;
namespace as = boost::asio;

BOOST_AUTO_TEST_CASE(tc) {
    auto version = am::protocol_version::v3_1_1;
    as::io_context ioc;
    auto ep = am::endpoint<async_mqtt::role::client, async_mqtt::stub_socket>::create(
        version,
        // for stub_socket args
        version,
        ioc
    );
    auto p = am::v3_1_1::connack_packet{
        true,   // session_present
        am::connect_return_code::not_authorized
    };
    // static_assert fail as expected
    auto ec = ep->send(p, as::use_future).get();
    BOOST_TEST(!ec);
}
