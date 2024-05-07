// Copyright Takatoshi Kondo 2024
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PREDEFINED_LAYER_CUSTOMIZE_HPP)
#define ASYNC_MQTT_PREDEFINED_LAYER_CUSTOMIZE_HPP

#include <boost/asio.hpp>

#include <async_mqtt/constant.hpp>

#if defined(ASYNC_MQTT_USE_TLS)
#include <async_mqtt/tls.hpp>
#endif // defined(ASYNC_MQTT_USE_TLS)

#include <async_mqtt/stream_traits.hpp>
#include <async_mqtt/log.hpp>
#include <async_mqtt/util/move.hpp>
#include <async_mqtt/predefined_underlying_layer.hpp>

/// @file

namespace async_mqtt {

namespace as = boost::asio;

#if defined(ASYNC_MQTT_USE_WS)

namespace bs = boost::beast;

template <typename NextLayer>
struct layer_customize<bs::websocket::stream<NextLayer>> {
    static void initialize(bs::websocket::stream<NextLayer>& ws) {
        ws.binary(true);
        ws.set_option(
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
        as::any_io_executor /* exe */,
        bs::websocket::stream<NextLayer>& ws,
        ConstBufferSequence const& cbs,
        CompletionToken&& token
    ) {
        // if just forwarding token to the underlying layer,
        // no as::bind_executor(exe, ...) is required.
        return ws.async_write(cbs, std::forward<CompletionToken>(token));
    }

    template <
        typename CompletionToken
    >
    static auto
    async_close(
        as::any_io_executor exe,
        bs::websocket::stream<NextLayer>& ws,
        CompletionToken&& token
    ) {
        return as::async_initiate<
            CompletionToken,
            void(error_code const& ec)
        > (
            [] (auto completion_handler, as::any_io_executor exe, bs::websocket::stream<NextLayer>& ws) {
                auto ch_sp = std::make_shared<decltype(completion_handler)>(force_move(completion_handler));
                ws.async_close(
                    bs::websocket::close_code::none,
                    // You must bind exe to the handler if you use underlying async_function
                    // using the completion handler that you defined.
                    as::bind_executor(
                        exe,
                        [exe, ch_sp]
                        (error_code const& ec_ws) mutable {
                            force_move(*ch_sp)(ec_ws);
                        }
                    )
                );
            },
            token,
            exe,
            std::ref(ws)
        );
    }
#if 0
    template <typename CompletionToken>
    static void do_read(
        as::any_io_executor exe,
        bs::websocket::stream<NextLayer>& ws,
        CompletionToken&& completion_handler
    ) {
        auto buffer = std::make_shared<bs::flat_buffer>();
        auto ch_sp = std::make_shared<CompletionToken>(force_move(completion_handler));
        ws.async_read(
            *buffer,
            // You must bind exe to the handler if you use underlying async_function
            // using the completion handler that you defined.
            as::bind_executor(
                exe,
                [exe, &ws, buffer, ch_sp]
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
                        force_move(*ch_sp)(ec_read);
                    }
                    else {
                        do_read(exe, ws, force_move(*ch_sp));
                    }
                }
            )
        );

    }
#endif
};

#endif // defined(ASYNC_MQTT_USE_WS)

#if defined(ASYNC_MQTT_USE_TLS)

template <typename NextLayer>
struct layer_customize<tls::stream<NextLayer>> {
    template <
        typename CompletionToken
    >
    static auto
    async_close(
        as::any_io_executor exe,
        tls::stream<NextLayer>& stream,
        CompletionToken&& token
    ) {
        return as::async_initiate<
            CompletionToken,
            void(error_code const& ec)
        > (
            [] (auto completion_handler, as::any_io_executor exe, tls::stream<NextLayer>& stream) {
                auto tim = std::make_shared<as::steady_timer>(
                    exe,
                    shutdown_timeout
                );
                auto ch_sp = std::make_shared<decltype(completion_handler)>(force_move(completion_handler));
                tim->async_wait(
                    // You must bind exe to the handler if you use underlying async_function
                    // using the completion handler that you defined.
                    as::bind_executor(
                        exe,
                        [wp = std::weak_ptr<as::steady_timer>(tim), ch_sp]
                        (error_code const& ec) mutable {
                            if (!ec) {
                                if (auto sp = wp.lock()) {
                                    ASYNC_MQTT_LOG("mqtt_impl", info)
                                        << "TLS async_shutdown timeout";
                                    force_move(*ch_sp)(ec);
                                }
                            }
                        }
                    )
                );
                stream.async_shutdown(
                    // You must bind exe to the handler if you use underlying async_function
                    // using the completion handler that you defined.
                    as::bind_executor(
                        exe,
                        [tim, ch_sp]
                        (error_code const& ec_shutdown) mutable {
                            ASYNC_MQTT_LOG("mqtt_impl", info)
                                << "TLS async_shutdown ec:" << ec_shutdown.message();
                            force_move(*ch_sp)(ec_shutdown);
                        }
                    )
                );
            },
            token,
            exe,
            std::ref(stream)
        );
    }
};

#endif // defined(ASYNC_MQTT_USE_TLS)

} // namespace async_mqtt

#endif // ASYNC_MQTT_PREDEFINED_LAYER_CUSTOMIZE_HPP
