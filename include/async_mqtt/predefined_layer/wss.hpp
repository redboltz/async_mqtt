// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PREDEFINED_LAYER_WSS_HPP)
#define ASYNC_MQTT_PREDEFINED_LAYER_WSS_HPP

#include <boost/asio.hpp>
#include <boost/beast/websocket/ssl.hpp>

#include <async_mqtt/predefined_layer/tls.hpp>
#include <async_mqtt/predefined_layer/ws.hpp>

/// @file

namespace async_mqtt {

namespace as = boost::asio;

namespace protocol {

namespace detail {
using mqtts_beast_workaround = tls::stream<mqtt_beast_workaround>;
} // namespace detail

/**
 * @breif Type alias of Boost.Beast WebSocket on TLS stream
 */
using wss = bs::websocket::stream<detail::mqtts_beast_workaround>;


} // namespace protocol


} // namespace async_mqtt

#endif // ASYNC_MQTT_PREDEFINED_LAYER_WSS_HPP
