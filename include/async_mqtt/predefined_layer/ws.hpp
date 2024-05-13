// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PREDEFINED_LAYER_WS_HPP)
#define ASYNC_MQTT_PREDEFINED_LAYER_WS_HPP

#include <async_mqtt/predefined_layer/mqtt.hpp>
#include <async_mqtt/predefined_layer/customized_websocket_stream.hpp>

/// @file

namespace async_mqtt {

namespace as = boost::asio;
namespace bs = boost::beast;

namespace protocol {

/**
 * @breif Type alias of Boost.Beast WebScoket
 */
using ws = bs::websocket::stream<mqtt>;

} // namespace protocol

template <
    typename NextLayer,
    typename CompletionToken = as::default_completion_token_t<
        typename bs::websocket::stream<NextLayer>::executor_type
    >
>
BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(
    CompletionToken,
    void(error_code)
)
underlying_handshake(
    bs::websocket::stream<NextLayer>& layer,
    std::string_view host,
    std::string_view port,
    std::string_view path,
    CompletionToken&& token = as::default_completion_token_t<
        typename bs::websocket::stream<NextLayer>::executor_type
    >{}
);

template <
    typename NextLayer,
    typename CompletionToken = as::default_completion_token_t<
        typename bs::websocket::stream<NextLayer>::executor_type
    >
>
BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(
    CompletionToken,
    void(error_code)
)
underlying_handshake(
    bs::websocket::stream<NextLayer>& layer,
    std::string_view host,
    std::string_view port,
    CompletionToken&& token = as::default_completion_token_t<
        typename bs::websocket::stream<NextLayer>::executor_type
    >{}
);

} // namespace async_mqtt

#include <async_mqtt/predefined_layer/impl/ws_handshake.hpp>

#endif // ASYNC_MQTT_PREDEFINED_LAYER_WS_HPP
