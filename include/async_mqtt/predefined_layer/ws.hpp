// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PREDEFINED_LAYER_WS_HPP)
#define ASYNC_MQTT_PREDEFINED_LAYER_WS_HPP

#include <async_mqtt/predefined_layer/customized_websocket_stream.hpp>

/// @file

namespace async_mqtt {

namespace as = boost::asio;
namespace bs = boost::beast;

namespace protocol {

// When https://github.com/boostorg/beast/issues/2775 would be fixed,
// this detail part would be erased, and detail::mqtt_beast_workaround is
// replaced with `mqtt`
namespace detail {
using mqtt_beast_workaround = as::basic_stream_socket<as::ip::tcp, as::io_context::executor_type>;
} // namespace detail

/**
 * @breif Type alias of Boost.Beast WebScoket
 */
using ws = bs::websocket::stream<detail::mqtt_beast_workaround>;

} // namespace protocol

} // namespace async_mqtt

#endif // ASYNC_MQTT_PREDEFINED_LAYER_WS_HPP
