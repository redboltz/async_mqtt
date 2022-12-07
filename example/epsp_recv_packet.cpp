// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <functional>

#include <boost/asio.hpp>

#include <async_mqtt/endpoint_variant.hpp>
#include <async_mqtt/protocol_version.hpp>
#include <async_mqtt/buffer_to_packet_variant.hpp>
#include <async_mqtt/predefined_underlying_layer.hpp>

namespace as = boost::asio;
namespace am = async_mqtt;

int main() {
    as::io_context ioc;
    as::ip::address address = boost::asio::ip::address::from_string("127.0.0.1");
    as::ip::tcp::endpoint endpoint{address, 1883};

    using epsp_t = am::endpoint_sp_variant<
        async_mqtt::role::server,
        async_mqtt::protocol::mqtt
    >;

    auto epsp = epsp_t::create(
        am::protocol_version::undetermined,
        am::protocol::mqtt{ioc.get_executor()}
    );
    as::ip::tcp::acceptor ac{ioc, endpoint};

    ioc.run();
}
