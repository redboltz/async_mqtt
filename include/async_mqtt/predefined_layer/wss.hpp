// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PREDEFINED_LAYER_WSS_HPP)
#define ASYNC_MQTT_PREDEFINED_LAYER_WSS_HPP

#include <boost/beast/ssl/ssl_stream.hpp>

#include <async_mqtt/predefined_layer/customized_ssl_stream.hpp>
#include <async_mqtt/predefined_layer/ws.hpp>

/// @file

namespace async_mqtt {

namespace as = boost::asio;

namespace protocol {

// When https://github.com/boostorg/beast/issues/2775 would be fixed,
// this detail part would be erased, and detail::mqtts_beast_workaround is
// replaced with `mqtts`
namespace detail {
using mqtts_beast_workaround = as::ssl::stream<mqtt_beast_workaround>;
} // namespace detail

/**
 * @breif Type alias of Boost.Beast WebSocket on TLS stream
 */
using wss = bs::websocket::stream<detail::mqtts_beast_workaround>;

} // namespace protocol

} // namespace async_mqtt

#endif // ASYNC_MQTT_PREDEFINED_LAYER_WSS_HPP
