// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PREDEFINED_LAYER_MQTTS_HPP)
#define ASYNC_MQTT_PREDEFINED_LAYER_MQTTS_HPP

#include <async_mqtt/predefined_layer/mqtt.hpp>
#include <async_mqtt/predefined_layer/customized_ssl_stream.hpp>

/// @file

namespace async_mqtt {

namespace as = boost::asio;
namespace tls = as::ssl; // for backword compatilibity

namespace protocol {

using mqtts = as::ssl::stream<mqtt>;

} // namespace protocol

template <
    typename NextLayer,
    typename CompletionToken = as::default_completion_token_t<
        typename as::ssl::stream<NextLayer>::executor_type
    >
>
BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(
    CompletionToken,
    void(error_code)
)
underlying_handshake(
    as::ssl::stream<NextLayer>& layer,
    std::string_view host,
    std::string_view port,
    CompletionToken&& token = as::default_completion_token_t<
        typename as::ssl::stream<NextLayer>::executor_type
    >{}
);

} // namespace async_mqtt

#include <async_mqtt/predefined_layer/impl/mqtts_handshake.hpp>

#endif // ASYNC_MQTT_PREDEFINED_LAYER_MQTTS_HPP
