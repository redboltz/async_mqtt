// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PREDEFINED_LAYER_WSS_HPP)
#define ASYNC_MQTT_PREDEFINED_LAYER_WSS_HPP

#include <boost/beast/ssl/ssl_stream.hpp>

#include <async_mqtt/predefined_layer/mqtts.hpp>
#include <async_mqtt/predefined_layer/ws.hpp>
#include <async_mqtt/predefined_layer/customized_websocket_stream.hpp>

/**
 * @defgroup predefined_layer_wss
 * @ingroup predefined_layer
 */

namespace async_mqtt {

namespace as = boost::asio;
namespace bs = boost::beast;

namespace protocol {

/**
 * @ingroup predefined_layer_wss
 * @brief Type alias of boost::beast::websocket::stream of mqtts
 * underlying_handshake function can be called with wss.
 * You can call the following functions to handshake.
 *
 * underlying_handshake(
 *     bs::websocket::stream<NextLayer>& layer,
 *     std::string_view host,
 *     std::string_view port,
 *     std::string_view path,
 *     CompletionToken&& token
 * )
 *
 * underlying_handshake(
 *     bs::websocket::stream<NextLayer>& layer,
 *     std::string_view host,
 *     std::string_view port,
 *     CompletionToken&& token
 * )
 */
using wss = bs::websocket::stream<mqtts>;

} // namespace protocol

} // namespace async_mqtt

#endif // ASYNC_MQTT_PREDEFINED_LAYER_WSS_HPP
