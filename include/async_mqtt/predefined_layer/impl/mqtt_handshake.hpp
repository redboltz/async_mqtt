// Copyright Takatoshi Kondo 2024
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PREDEFINED_LAYER_IMPL_MQTT_HANDSHAKE_HPP)
#define ASYNC_MQTT_PREDEFINED_LAYER_IMPL_MQTT_HANDSHAKE_HPP

#include <async_mqtt/predefined_layer/mqtt.hpp>

namespace async_mqtt {
namespace as = boost::asio;

template <
    typename Socket,
    typename Executor
>
struct mqtt_handshake_op {
    mqtt_handshake_op(
        as::basic_stream_socket<Socket, Executor>& layer,
        std::string_view host,
        std::string_view port
    ):layer{layer},
      host{host},
      port{port}
    {}

    as::basic_stream_socket<Socket, Executor>& layer;
    std::string_view host;
    std::string_view port;
    enum { dispatch, resolve, connect, complete } state = dispatch;

    template <typename Self>
    void operator()(
        Self& self
    ) {
        if (state == dispatch) {
            state = resolve;
            auto& a_layer{layer};
            as::dispatch(
                a_layer.get_executor(),
                force_move(self)
            );
        }
        else {
            BOOST_ASSERT(state == resolve);
            state = connect;
            auto res = std::make_shared<as::ip::tcp::resolver>(layer.get_executor());
            res->async_resolve(
                host,
                port,
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
        auto& a_layer{layer};
        as::async_connect(
            a_layer,
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
    typename Socket,
    typename Executor,
    typename CompletionToken
>
BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(
    CompletionToken,
    void(error_code)
)
async_underlying_handshake(
    as::basic_stream_socket<Socket, Executor>& layer,
    std::string_view host,
    std::string_view port,
    CompletionToken&& token
) {
    return
        as::async_compose<
            CompletionToken,
            void(error_code)
        >(
            mqtt_handshake_op{
                layer,
                host,
                port
            },
            token,
            layer
        );
}

} // namespace async_mqtt

#endif // ASYNC_MQTT_PREDEFINED_LAYER_IMPL_MQTT_HANDSHAKE_HPP
