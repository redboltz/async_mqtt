// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PREDEFINED_LAYER_BASIC_STREAM_HPP)
#define ASYNC_MQTT_PREDEFINED_LAYER_BASIC_STREAM_HPP

#include <boost/asio.hpp>

#include <async_mqtt/stream_traits.hpp>
#include <async_mqtt/log.hpp>

/// @file

namespace async_mqtt {

namespace as = boost::asio;

template <typename Protocol, typename Executor>
struct layer_customize<as::basic_stream_socket<Protocol, Executor>> {
    template <
        typename CompletionToken
    >
    static auto
    async_close(
        as::any_io_executor exe,
        as::basic_stream_socket<Protocol, Executor>& stream,
        CompletionToken&& token
    ) {
        return as::async_initiate<
            CompletionToken,
            void(error_code const& ec)
        > (
            [] (auto completion_handler,
                as::any_io_executor /* exe */,
                as::basic_stream_socket<Protocol, Executor>& stream
            ) {
                error_code ec;
                if (stream.is_open()) {
                    ASYNC_MQTT_LOG("mqtt_impl", info)
                        << "TCP close";
                    stream.close(ec);
                }
                else {
                    ASYNC_MQTT_LOG("mqtt_impl", info)
                        << "TCP already closed";
                }
                force_move(completion_handler)(ec);
            },
            token,
            exe,
            std::ref(stream)
        );
    }
};

} // namespace async_mqtt

#endif // ASYNC_MQTT_PREDEFINED_LAYER_BASIC_STREAM_HPP
