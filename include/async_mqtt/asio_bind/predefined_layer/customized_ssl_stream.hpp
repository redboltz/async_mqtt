// Copyright Takatoshi Kondo 2024
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PREDEFINED_LAYER_CUSTOMIZED_SSL_STREAM_HPP)
#define ASYNC_MQTT_PREDEFINED_LAYER_CUSTOMIZED_SSL_STREAM_HPP

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

#include <async_mqtt/asio_bind/stream_customize.hpp>
#include <async_mqtt/util/log.hpp>

/// @file

namespace async_mqtt {

namespace as = boost::asio;
namespace tls = as::ssl; // for backword compatilibity

static constexpr auto shutdown_timeout = std::chrono::seconds(3);

/**
 * @brief customization class template specialization for boost::asio::ssl::stream
 *
 * @see
 *   <a href="../../customize.html">Layor customize</a>
 */
template <typename NextLayer>
struct layer_customize<as::ssl::stream<NextLayer>> {
    template <
        typename CompletionToken = as::default_completion_token_t<typename as::ssl::stream<NextLayer>::executor_type>
    >
    static auto
    async_handshake(
        as::ssl::stream<NextLayer>& stream,
        std::string_view host,
        std::string_view port,
        CompletionToken&& token = as::default_completion_token_t<typename as::ssl::stream<NextLayer>::executor_type>{}
    ) {
        return
            as::async_compose<
                CompletionToken,
            void(error_code)
        >(
            handshake_op{
                stream,
                host,
                port
            },
            token,
            stream
        );
    }

    struct handshake_op {
        handshake_op(
            as::ssl::stream<NextLayer>& stream,
            std::string_view host,
            std::string_view port
        ):stream{stream},
          host{host},
          port{port}
        {}

        as::ssl::stream<NextLayer>& stream;
        std::string host;
        std::string port;
        enum { dispatch, under, handshake, complete } state = dispatch;

        template <typename Self>
        void operator()(
            Self& self
        ) {
            if (state == dispatch) {
                state = under;
                auto& a_stream{stream};
                as::dispatch(
                    a_stream.get_executor(),
                    force_move(self)
                );
            }
            else {
                BOOST_ASSERT(state == under);
                state = handshake;
                auto& a_stream{stream};
                auto a_host{host};
                auto a_port{port};
                layer_customize<NextLayer>::async_handshake(
                    a_stream.next_layer(),
                    a_host,
                    a_port,
                    force_move(self)
                );
            }
        }

        template <typename Self>
        void operator()(
            Self& self,
            error_code ec
        ) {
            if (state == handshake) {
                state = complete;
                if (ec) {
                    self.complete(ec);
                    return;
                }
                auto& a_stream{stream};
                a_stream.async_handshake(
                    as::ssl::stream_base::client,
                    force_move(self)
                );
            }
            else {
                BOOST_ASSERT(state == complete);
                self.complete(ec);
            }
        }
    };

    template <
        typename CompletionToken
    >
    static auto
    async_close(
        as::ssl::stream<NextLayer>& stream,
        CompletionToken&& token
    ) {
        return as::async_compose<
            CompletionToken,
            void(error_code const& ec)
        > (
            async_close_impl{
                stream
            },
            token,
            stream
        );
    }

    struct async_close_impl {
        as::ssl::stream<NextLayer>& stream;
        enum {
            shutdown,
            complete
        } state = shutdown;

        template <typename Self>
        void operator()(
            Self& self
        ) {
            BOOST_ASSERT(state == shutdown);
            auto tim = std::make_shared<as::steady_timer>(
                stream.get_executor(),
                shutdown_timeout
            );
            auto sig = std::make_shared<as::cancellation_signal>();
            tim->async_wait(
                [sig, wp = std::weak_ptr<as::steady_timer>(tim)]
                (error_code const& ec) {
                    if (!ec) {
                        if (auto sp = wp.lock()) {
                            ASYNC_MQTT_LOG("mqtt_impl", info)
                                << "TLS async_shutdown timeout";
                            sig->emit(as::cancellation_type::terminal);
                        }
                    }
                }
            );
            auto& a_stream{stream};
            a_stream.async_shutdown(
                as::bind_cancellation_slot(
                    sig->slot(),
                    as::consign(
                        force_move(self),
                        tim,
                        sig
                    )
                )
            );
        }

        template <typename Self>
        void operator()(
            Self& self,
            error_code const& ec
        ) {
            ASYNC_MQTT_LOG("mqtt_impl", info)
                << "TLS async_shutdown ec:" << ec.message();
            state = complete;
            self.complete(ec);
        }
    };
};

} // namespace async_mqtt

#endif // ASYNC_MQTT_PREDEFINED_LAYER_CUSTOMIZED_SSL_STREAM_HPP
