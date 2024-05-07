// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PREDEFINED_UNDERLYING_LAYER_HPP)
#define ASYNC_MQTT_PREDEFINED_UNDERLYING_LAYER_HPP

#include <boost/asio.hpp>

#if defined(ASYNC_MQTT_USE_WS)
#include <boost/beast/websocket/stream.hpp>
#include <boost/beast/websocket/error.hpp>
#endif // defined(ASYNC_MQTT_USE_WS)

#include <async_mqtt/constant.hpp>

#if defined(ASYNC_MQTT_USE_TLS)

#include <async_mqtt/tls.hpp>

#if defined(ASYNC_MQTT_USE_WS) && !defined(ASYNC_MQTT_TLS_WS_INCLUDE)
#define ASYNC_MQTT_TLS_WS_INCLUDE <boost/beast/websocket/ssl.hpp>
#endif // defined(ASYNC_MQTT_USE_WS) && !defined(ASYNC_MQTT_TLS_WS_INCLUDE)

#include ASYNC_MQTT_TLS_WS_INCLUDE

#endif // defined(ASYNC_MQTT_USE_TLS)

/// @file

namespace async_mqtt {

namespace as = boost::asio;

#if defined(ASYNC_MQTT_USE_WS)
namespace bs = boost::beast;
#endif //defined(ASYNC_MQTT_USE_WS)

namespace protocol {

/**
 * @breif Type alias of Boost.Asio TCP socket
 */
using mqtt = as::basic_stream_socket<as::ip::tcp, as::any_io_executor>;

#if defined(ASYNC_MQTT_USE_WS)

namespace detail {
using mqtt_beast_workaround = as::basic_stream_socket<as::ip::tcp, as::io_context::executor_type>;
} // namespace detail

/**
 * @breif Type alias of Boost.Beast WebScoket
 */
using ws = bs::websocket::stream<detail::mqtt_beast_workaround>;

#endif //defined(ASYNC_MQTT_USE_WS)


#if defined(ASYNC_MQTT_USE_TLS)

/**
 * @breif Type alias of TLS stream
 */
using mqtts = tls::stream<mqtt>;

#if defined(ASYNC_MQTT_USE_WS)

namespace detail {
using mqtts_beast_workaround = tls::stream<mqtt_beast_workaround>;
} // namespace detail

/**
 * @breif Type alias of Boost.Beast WebSocket on TLS stream
 */
using wss = bs::websocket::stream<detail::mqtts_beast_workaround>;

#endif // defined(ASYNC_MQTT_USE_WS)
#endif // defined(ASYNC_MQTT_USE_TLS)

} // namespace protocol
} // namespace async_mqtt

#endif // ASYNC_MQTT_PREDEFINED_UNDERLYING_LAYER_HPP
