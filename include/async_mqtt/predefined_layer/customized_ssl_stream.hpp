// Copyright Takatoshi Kondo 2024
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PREDEFINED_LAYER_CUSTOMIZED_SSL_STREAM_HPP)
#define ASYNC_MQTT_PREDEFINED_LAYER_CUSTOMIZED_SSL_STREAM_HPP

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

#include <async_mqtt/stream_traits.hpp>
#include <async_mqtt/log.hpp>
#include <async_mqtt/constant.hpp>

/// @file

namespace async_mqtt {

namespace as = boost::asio;
namespace tls = as::ssl; // for backword compatilibity

template <typename NextLayer>
struct layer_customize<as::ssl::stream<NextLayer>> {
    template <
        typename CompletionToken
    >
    static auto
    async_close(
        as::any_io_executor exe,
        as::ssl::stream<NextLayer>& stream,
        CompletionToken&& token
    ) {
        return as::async_initiate<
            CompletionToken,
            void(error_code const& ec)
        > (
            [] (auto completion_handler,
                as::any_io_executor exe,
                as::ssl::stream<NextLayer>& stream
            ) {
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

} // namespace async_mqtt

#endif // ASYNC_MQTT_PREDEFINED_LAYER_CUSTOMIZED_SSL_STREAM_HPP
