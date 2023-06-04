// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PREDEFINED_UNDERLYING_LAYER_HPP)
#define ASYNC_MQTT_PREDEFINED_UNDERLYING_LAYER_HPP

#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/websocket/stream.hpp>

#include <async_mqtt/tls.hpp>

/// @file

namespace async_mqtt {

namespace as = boost::asio;
namespace bs = boost::beast;

namespace protocol {

/**
 * @breif Type alias of Boost.Asio TCP socket
 */
using mqtt = as::basic_stream_socket<as::ip::tcp, as::io_context::executor_type>;

#if defined(ASYNC_MQTT_USE_WS)
/**
 * @breif Type alias of Boost.Beast WebScoket
 */
using ws = bs::websocket::stream<mqtt>;
#endif //defined(ASYNC_MQTT_USE_WS)

} // namespace procotol

} // namespace async_mqtt

#if defined(ASYNC_MQTT_USE_TLS)

namespace async_mqtt {

namespace protocol {

/**
 * @breif Type alias of TLS stream
 */
using mqtts = tls::stream<mqtt>;

#if defined(ASYNC_MQTT_USE_WS)
/**
 * @breif Type alias of Boost.Beast WebSocket on TLS stream
 */
using wss = bs::websocket::stream<mqtts>;
#endif // defined(ASYNC_MQTT_USE_WS)


} // namespace procotol

} // namespace async_mqtt

#endif // defined(ASYNC_MQTT_USE_TLS)

#endif // ASYNC_MQTT_PREDEFINED_UNDERLYING_LAYER_HPP
