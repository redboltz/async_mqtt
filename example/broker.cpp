// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <async_mqtt/broker/broker.hpp>
#include <async_mqtt/predefined_underlying_layer.hpp>

namespace am = async_mqtt;
namespace as = boost::asio;

int main() {
    as::io_context ioc;
    am::broker<
        am::protocol::mqtt,
        am::protocol::ws
#if defined(ASYNC_MQTT_USE_TLS)
        ,
        am::protocol::mqtts,
        am::protocol::wss
#endif // defined(ASYNC_MQTT_USE_TLS)
    > brk{ioc};
}
