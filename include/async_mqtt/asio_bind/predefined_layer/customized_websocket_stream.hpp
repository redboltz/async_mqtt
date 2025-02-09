// Copyright Takatoshi Kondo 2024
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_ASIO_BIND_PREDEFINED_LAYER_CUSTOMIZED_WEBSOCKET_STREAM_HPP)
#define ASYNC_MQTT_ASIO_BIND_PREDEFINED_LAYER_CUSTOMIZED_WEBSOCKET_STREAM_HPP

#include <boost/asio.hpp>
#include <boost/beast/websocket/stream.hpp>

#include <async_mqtt/asio_bind/stream_customize.hpp>
#include <async_mqtt/util/log.hpp>

/// @file

namespace async_mqtt {

namespace as = boost::asio;
namespace bs = boost::beast;

/**
 * @brief customization class template specialization for boost::beast::websocket::stream
 *
 * @see
 *   <a href="../../customize.html">Layor customize</a>
 */
template <typename NextLayer>
struct layer_customize<bs::websocket::stream<NextLayer>> {

    // initialize

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

    // async_handshake

    template <
        typename CompletionToken = as::default_completion_token_t<typename bs::websocket::stream<NextLayer>::executor_type>
    >
    static auto
    async_handshake(
        bs::websocket::stream<NextLayer>& stream,
        std::string_view host,
        std::string_view port,
        std::string_view path,
        CompletionToken&& token = as::default_completion_token_t<typename bs::websocket::stream<NextLayer>::executor_type>{}
    ) {
        return
            as::async_compose<
                CompletionToken,
            void(error_code)
        >(
            handshake_op{
                stream,
                host,
                port,
                path
            },
            token,
            stream
        );
    }

    template <
        typename CompletionToken
    >
    static auto
    async_handshake(
        bs::websocket::stream<NextLayer>& stream,
        std::string_view host,
        std::string_view port,
        CompletionToken&& token
    ) {
        return
            as::async_compose<
                CompletionToken,
            void(error_code)
        >(
            handshake_op{
                stream,
                host,
                port,
                "/"
            },
            token,
            stream
        );
    }

    struct handshake_op {
        handshake_op(
            bs::websocket::stream<NextLayer>& stream,
            std::string_view host,
            std::string_view port,
            std::string_view path
        ):stream{stream},
          host{host},
          port{port},
          path{path}
        {}

        bs::websocket::stream<NextLayer>& stream;
        std::string host;
        std::string port;
        std::string path;
        enum {dispatch, under, handshake, complete} state = dispatch;

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
                auto a_host{host};
                auto a_path{path};
                a_stream.async_handshake(
                    a_host,
                    a_path,
                    force_move(self)
                );
            }
            else {
                BOOST_ASSERT(state == complete);
                self.complete(ec);
            }
        }
    };

    // async_write

    template <
        typename ConstBufferSequence,
        typename CompletionToken
    >
    static auto
    async_write(
        bs::websocket::stream<NextLayer>& stream,
        ConstBufferSequence const& cbs,
        CompletionToken&& token
    ) {
        return as::async_compose<
            CompletionToken,
            void(error_code const& ec, std::size_t size)
        > (
            async_write_impl<ConstBufferSequence>{
                stream,
                cbs
            },
            token,
            stream
        );
    }

    template <typename ConstBufferSequence>
    struct async_write_impl {
        bs::websocket::stream<NextLayer>& stream;
        ConstBufferSequence const& cbs;

        template <typename Self>
        void operator()(
            Self& self
        ) {
            stream.async_write(
                cbs,
                force_move(self)
            );
        }

        template <typename Self>
        void operator()(
            Self& self,
            error_code const& ec,
            std::size_t size
        ) {
            self.complete(ec, size);
        }
    };

    // async_close

    template <
        typename CompletionToken
    >
    static auto
    async_close(
        bs::websocket::stream<NextLayer>& stream,
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
        bs::websocket::stream<NextLayer>& stream;

        template <typename Self>
        void operator()(
            Self& self
        ) {
            stream.async_close(
                bs::websocket::close_code::none,
                force_move(self)
            );
        }

        template <typename Self>
        void operator()(
            Self& self,
            error_code const& ec
        ) {
            if (ec) {
                self.complete(ec);
            }
            else {
                auto buffer = std::make_shared<bs::flat_buffer>();
                stream.async_read(
                    *buffer,
                    as::consign(
                        force_move(self),
                        buffer
                    )
                );
            }
        }

        template <typename Self>
        void operator()(
            Self& self,
            error_code const& ec,
            std::size_t /* size */
        ) {
            if (ec) {
                if (ec == bs::websocket::error::closed) {
                    ASYNC_MQTT_LOG("mqtt_impl", info)
                        << "ws async_read (for close)  success";
                }
                else {
                    ASYNC_MQTT_LOG("mqtt_impl", info)
                        << "ws async_read (for close):" << ec.message();
                }
                self.complete(ec);
            }
            else {
                auto buffer = std::make_shared<bs::flat_buffer>();
                stream.async_read(
                    *buffer,
                    as::consign(
                        force_move(self),
                        buffer
                    )
                );
            }
        }
    };
};

} // namespace async_mqtt

#endif // ASYNC_MQTT_ASIO_BIND_PREDEFINED_LAYER_CUSTOMIZED_WEBSOCKET_STREAM_HPP
