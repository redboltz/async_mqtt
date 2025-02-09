// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PREDEFINED_LAYER_MQTTS_HPP)
#define ASYNC_MQTT_PREDEFINED_LAYER_MQTTS_HPP

#include <async_mqtt/predefined_layer/mqtt.hpp>
#include <async_mqtt/predefined_layer/customized_ssl_stream.hpp>

namespace async_mqtt {

namespace as = boost::asio;
namespace tls = as::ssl; // for backword compatilibity

namespace protocol {

/**
 * @brief Type alias of boost::asio::ssl::stream of mqtt
 * @brief predefined underlying layer (TLS)
 *
 */
using mqtts = as::ssl::stream<mqtt>;


/**
 * @brief TLS handshake
 * This function does underlying layers handshaking prior to TLS handshake
 * @param layer  TLS layer
 * @param host   host name or IP address to connect
 * @param port   port number to connect
 * @param token  completion token. signature is void(error_code)
 * @return deduced by token
 *
 */
template <
    typename NextLayer,
    typename CompletionToken = as::default_completion_token_t<
        typename as::ssl::stream<NextLayer>::executor_type
    >
>
auto
async_underlying_handshake(
    as::ssl::stream<NextLayer>& layer,
    std::string_view host,
    std::string_view port,
    CompletionToken&& token = as::default_completion_token_t<
        typename as::ssl::stream<NextLayer>::executor_type
    >{}
);

} // namespace protocol

} // namespace async_mqtt

#endif // ASYNC_MQTT_PREDEFINED_LAYER_MQTTS_HPP
