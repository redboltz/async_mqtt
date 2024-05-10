// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PREDEFINED_LAYER_WSS_HPP)
#define ASYNC_MQTT_PREDEFINED_LAYER_WSS_HPP

#include <boost/beast/ssl/ssl_stream.hpp>

#include <async_mqtt/predefined_layer/mqtts.hpp>
#include <async_mqtt/predefined_layer/customized_websocket_stream.hpp>

/// @file

namespace async_mqtt {

namespace as = boost::asio;
namespace bs = boost::beast;

namespace protocol {

/**
 * @breif Type alias of Boost.Beast WebSocket on TLS stream
 */
using wss = bs::websocket::stream<mqtts>;

} // namespace protocol

} // namespace async_mqtt

#endif // ASYNC_MQTT_PREDEFINED_LAYER_WSS_HPP
