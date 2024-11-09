// Copyright Takatoshi Kondo 2024
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PREDEFINED_LAYER_CUSTOMIZED_BASIC_STREAM_HPP)
#define ASYNC_MQTT_PREDEFINED_LAYER_CUSTOMIZED_BASIC_STREAM_HPP

#include <boost/asio.hpp>

#include <async_mqtt/util/stream_traits.hpp>
#include <async_mqtt/util/log.hpp>

namespace async_mqtt {

namespace as = boost::asio;

/**
 * @defgroup predefined_customize predefined underlying customize
 * @ingroup underlying_customize
 */

/**
 * @ingroup predefined_customize
 * @brief customization class template specialization for boost::asio::basic_stream_socket
 *
 * #### Requirements
 * @li Header: async_mqtt/predefined_layer/customized_basic_stream.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 * @see
 *   <a href="../../customize.html">Layor customize</a>
 */
template <typename Protocol, typename Executor>
struct layer_customize<as::basic_stream_socket<Protocol, Executor>> {
    static auto
    test(
        as::basic_stream_socket<Protocol, Executor>&,
        std::string_view,
        std::string_view
    ) {
        return 32;
    }

    template <
        typename CompletionToken
    >
    static auto
    test2(
        as::basic_stream_socket<Protocol, Executor>&,
        std::string_view,
        std::string_view,
        CompletionToken&&
    ) {
        return 32;
    }

    template <
        typename CompletionToken
    >
    static auto
    async_handshake(
        as::basic_stream_socket<Protocol, Executor>& stream,
        std::string_view host,
        std::string_view port,
        CompletionToken&& token = as::default_completion_token_t<Executor>{}
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
            as::basic_stream_socket<Protocol, Executor>& stream,
            std::string_view host,
            std::string_view port
        ):stream{stream},
          host{host},
          port{port}
        {}

        as::basic_stream_socket<Protocol, Executor>& stream;
        std::string host;
        std::string port;
        enum { dispatch, resolve, connect, complete } state = dispatch;

        template <typename Self>
        void operator()(
            Self& self
        ) {
            if (state == dispatch) {
                state = resolve;
                auto& a_stream{stream};
                as::dispatch(
                    a_stream.get_executor(),
                    force_move(self)
                );
            }
            else {
                BOOST_ASSERT(state == resolve);
                state = connect;
                auto res = std::make_shared<as::ip::tcp::resolver>(stream.get_executor());
                auto a_host{host};
                auto a_port{port};
                res->async_resolve(
                    a_host,
                    a_port,
                    as::consign(
                        force_move(self),
                        res
                    )
                );
            }
        }

        struct default_connect_condition {
            template <typename Endpoint>
            bool operator()(error_code const&, Endpoint const&) {
                return true;
            }
        };

        template <typename Self>
        void operator()(
            Self& self,
            error_code ec,
            as::ip::tcp::resolver::results_type eps
        ) {
            BOOST_ASSERT(state == connect);
            state = complete;
            if (ec) {
                self.complete(ec);
                return;
            }
            auto& a_stream{stream};
            as::async_connect(
                a_stream,
                eps,
                default_connect_condition(),
                force_move(self)
            );
        }

        template <typename Self>
        void operator()(
            Self& self,
            error_code ec,
            as::ip::tcp::endpoint /*unused*/
        ) {
            BOOST_ASSERT(state == complete);
            self.complete(ec);
        }
    };

    template <
        typename CompletionToken
    >
    static auto
    async_close(
        as::basic_stream_socket<Protocol, Executor>& stream,
        CompletionToken&& token
    ) {
        return as::async_compose<
            CompletionToken,
            void(error_code const& ec)
        > (
            [&stream](auto& self) {
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
                self.complete(ec);
            },
            token,
            stream
        );
    }
};

} // namespace async_mqtt

#endif // ASYNC_MQTT_PREDEFINED_LAYER_CUSTOMIZED_BASIC_STREAM_HPP
