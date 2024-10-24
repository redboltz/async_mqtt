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
 * @defgroup predefined_layer_wss predefined underlying layer (Websocket on TLS)
 * @ingroup predefined_layer
 *
 * #### Requirements
 * @li Header: async_mqtt/predefined_layer/wss.hpp
 *
 */

namespace async_mqtt {

namespace as = boost::asio;
namespace bs = boost::beast;

namespace protocol {

/**
 * @ingroup predefined_layer_wss
 * @brief Type alias of boost::beast::websocket::stream of mqtts
 * async_underlying_handshake function can be called with wss.
 * You can call the following functions to handshake.
 *
 * async_underlying_handshake(
 *     bs::websocket::stream<NextLayer>& layer,
 *     std::string_view host,
 *     std::string_view port,
 *     std::string_view path,
 *     CompletionToken&& token
 * )
 *
 * async_underlying_handshake(
 *     bs::websocket::stream<NextLayer>& layer,
 *     std::string_view host,
 *     std::string_view port,
 *     CompletionToken&& token
 * )
 *
 * #### Requirements
 * @li Header: async_mqtt/predefined_layer/wss.hpp
 *
 */
using wss = bs::websocket::stream<mqtts>;

} // namespace protocol

} // namespace async_mqtt

#endif // ASYNC_MQTT_PREDEFINED_LAYER_WSS_HPP
