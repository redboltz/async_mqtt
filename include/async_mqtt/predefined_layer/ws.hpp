// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PREDEFINED_LAYER_WS_HPP)
#define ASYNC_MQTT_PREDEFINED_LAYER_WS_HPP

#include <boost/asio.hpp>
#include <boost/beast/websocket/stream.hpp>

#include <async_mqtt/stream_traits.hpp>
#include <async_mqtt/log.hpp>

/// @file

namespace async_mqtt {

namespace as = boost::asio;
namespace bs = boost::beast;

namespace protocol {

namespace detail {
using mqtt_beast_workaround = as::basic_stream_socket<as::ip::tcp, as::io_context::executor_type>;
} // namespace detail

/**
 * @breif Type alias of Boost.Beast WebScoket
 */
using ws = bs::websocket::stream<detail::mqtt_beast_workaround>;

} // namespace protocol

template <typename NextLayer>
struct layer_customize<bs::websocket::stream<NextLayer>> {
    static void initialize(bs::websocket::stream<NextLayer>& stream) {
        stream.binary(true);
        stream.set_option(
            bs::websocket::stream_base::decorator(
                [](bs::websocket::request_type& req) {
                    req.set("Sec-WebSocket-Protocol", "mqtt");
                }
            )
        );
    }

    template <
        typename ConstBufferSequence,
        typename CompletionToken
    >
    static auto
    async_write(
        as::any_io_executor exe,
        bs::websocket::stream<NextLayer>& stream,
        ConstBufferSequence const& cbs,
        CompletionToken&& token
    ) {
        return as::async_initiate<
            CompletionToken,
            void(error_code const& ec, std::size_t)
        > (
            [] (auto completion_handler,
                as::any_io_executor exe,
                bs::websocket::stream<NextLayer>& stream,
                ConstBufferSequence const& cbs
            ) {
                stream.async_write(
                    cbs,
                    // You must bind exe to the handler if you use underlying async_function
                    // using the completion handler that you defined.
                    as::bind_executor(
                        exe,
                        [completion_handler = std::forward<CompletionToken>(completion_handler)]
                        (error_code const& ec_write, std::size_t size) mutable {
                            force_move(completion_handler)(ec_write, size);
                        }
                    )
                );
            },
            token,
            exe,
            std::ref(stream),
            std::ref(cbs)
        );
    }

    template <
        typename CompletionToken
    >
    static auto
    async_close(
        as::any_io_executor exe,
        bs::websocket::stream<NextLayer>& stream,
        CompletionToken&& token
    ) {
        return as::async_initiate<
            CompletionToken,
            void(error_code const& ec)
        > (
            [] (auto completion_handler,
                as::any_io_executor exe,
                bs::websocket::stream<NextLayer>& stream
            ) {
                stream.async_close(
                    bs::websocket::close_code::none,
                    // You must bind exe to the handler if you use underlying async_function
                    // using the completion handler that you defined.
                    as::bind_executor(
                        exe,
                        [exe, &stream, completion_handler = force_move(completion_handler)]
                        (error_code const& ec_stream) mutable {
                            if (ec_stream) {
                                force_move(completion_handler)(ec_stream);
                            }
                            else {
                                do_read(exe, stream, force_move(completion_handler));
                            }
                        }
                    )
                );
            },
            token,
            exe,
            std::ref(stream)
        );
    }

    template <typename CompletionToken>
    static void do_read(
        as::any_io_executor exe,
        bs::websocket::stream<NextLayer>& stream,
        CompletionToken&& completion_handler
    ) {
        auto buffer = std::make_shared<bs::flat_buffer>();
        stream.async_read(
            *buffer,
            // You must bind exe to the handler if you use underlying async_function
            // using the completion handler that you defined.
            as::bind_executor(
                exe,
                [exe, &stream, buffer, completion_handler = std::forward<CompletionToken>(completion_handler)]
                (error_code const& ec_read, std::size_t) mutable {
                    if (ec_read) {
                        if (ec_read == bs::websocket::error::closed) {
                            ASYNC_MQTT_LOG("mqtt_impl", info)
                                << "ws async_read (for close)  success";
                        }
                        else {
                            ASYNC_MQTT_LOG("mqtt_impl", info)
                                << "ws async_read (for close):" << ec_read.message();
                        }
                        force_move(completion_handler)(ec_read);
                    }
                    else {
                        do_read(exe, stream, force_move(completion_handler));
                    }
                }
            )
        );
    }
};

} // namespace async_mqtt

#endif // ASYNC_MQTT_PREDEFINED_LAYER_WS_HPP
