// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PREDEFINED_LAYER_MQTTS_HPP)
#define ASYNC_MQTT_PREDEFINED_LAYER_MQTTS_HPP

#include <async_mqtt/predefined_layer/mqtt.hpp>
#include <async_mqtt/predefined_layer/customized_ssl_stream.hpp>

/// @file

namespace async_mqtt {

namespace as = boost::asio;
namespace tls = as::ssl; // for backword compatilibity

namespace protocol {

using mqtts = as::ssl::stream<protocol::mqtt>;

} // namespace protocol

} // namespace async_mqtt

#endif // ASYNC_MQTT_PREDEFINED_LAYER_MQTTS_HPP
