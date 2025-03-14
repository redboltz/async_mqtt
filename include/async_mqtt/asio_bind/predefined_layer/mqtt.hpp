// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_ASIO_BIND_PREDEFINED_LAYER_MQTT_HPP)
#define ASYNC_MQTT_ASIO_BIND_PREDEFINED_LAYER_MQTT_HPP

#include <boost/asio.hpp>

#include <async_mqtt/asio_bind/predefined_layer/customized_basic_stream.hpp>

namespace async_mqtt {

namespace as = boost::asio;

namespace protocol {

/**
 * @brief Type alias of Boost.Asio TCP socket
 *
 */
using mqtt = as::basic_stream_socket<as::ip::tcp, as::any_io_executor>;


/**
 * @brief resovling name and connect TCP layer
 * @param layer  TCP layer
 * @param host   host name or IP address to connect
 * @param port   port number to connect
 * @param token  completion token. signature is void(error_code)
 * @return deduced by token
 *
 */
template <
    typename Socket,
    typename Executor,
    typename CompletionToken = as::default_completion_token_t<
        Executor
    >
>
auto
async_underlying_handshake(
    as::basic_stream_socket<Socket, Executor>& layer,
    std::string_view host,
    std::string_view port,
    CompletionToken&& token = as::default_completion_token_t<Executor>{}
);

} // namespace protocol

} // namespace async_mqtt

#endif // ASYNC_MQTT_ASIO_BIND_PREDEFINED_LAYER_MQTT_HPP
