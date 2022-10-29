// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PREDEFINED_UNDERLYING_LAYER_HPP)
#define ASYNC_MQTT_PREDEFINED_UNDERLYING_LAYER_HPP

#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/websocket/stream_fwd.hpp>

namespace async_mqtt {

namespace as = boost::asio;
namespace bs = boost::beast;

namespace protocol {

using mqtt = as::basic_stream_socket<as::ip::tcp, as::io_context::executor_type>;
using ws = bs::websocket::stream<mqtt>;

} // namespace procotol

} // namespace async_mqtt

#if defined(ASYNC_MQTT_USE_TLS)

#include <boost/asio/ssl.hpp>

namespace async_mqtt {

namespace protocol {

using mqtts = as::ssl::stream<mqtt>;
using wss = bs::websocket::stream<mqtts>;


} // namespace procotol

} // namespace async_mqtt

#endif // defined(ASYNC_MQTT_USE_TLS)

#endif // ASYNC_MQTT_PREDEFINED_UNDERLYING_LAYER_HPP
