// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PREDEFINED_LAYER_MQTT_HPP)
#define ASYNC_MQTT_PREDEFINED_LAYER_MQTT_HPP

#include <boost/asio.hpp>

#include <async_mqtt/predefined_layer/customized_basic_stream.hpp>

/// @file

namespace async_mqtt {

namespace as = boost::asio;

namespace protocol {

/**
 * @breif Type alias of Boost.Asio TCP socket
 */
using mqtt = as::basic_stream_socket<as::ip::tcp, as::any_io_executor>;

} // namespace protocol

template <
    typename Socket,
    typename Executor,
    typename CompletionToken = as::default_completion_token_t<
        Executor
    >
>
BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(
    CompletionToken,
    void(error_code)
)
underlying_handshake(
    as::basic_stream_socket<Socket, Executor>& layer,
    std::string_view host,
    std::string_view port,
    CompletionToken&& token = as::default_completion_token_t<Executor>{}
);

} // namespace async_mqtt

#include <async_mqtt/predefined_layer/impl/mqtt_handshake.hpp>

#endif // ASYNC_MQTT_PREDEFINED_LAYER_MQTT_HPP
