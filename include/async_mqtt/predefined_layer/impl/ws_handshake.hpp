// Copyright Takatoshi Kondo 2024
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PREDEFINED_LAYER_IMPL_WS_HANDSHAKE_HPP)
#define ASYNC_MQTT_PREDEFINED_LAYER_IMPL_WS_HANDSHAKE_HPP

#include <async_mqtt/predefined_layer/ws.hpp>

namespace async_mqtt {
namespace as = boost::asio;

namespace protocol {

template <typename NextLayer>
struct ws_handshake_op {
    ws_handshake_op(
        bs::websocket::stream<NextLayer>& layer,
        std::string_view host,
        std::string_view port,
        std::string_view path
    ):layer{layer},
      host{host},
      port{port},
      path{path}
    {}

    bs::websocket::stream<NextLayer>& layer;
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
            auto& a_layer{layer};
            as::dispatch(
                a_layer.get_executor(),
                force_move(self)
            );
        }
        else {
            BOOST_ASSERT(state == under);
            state = handshake;
            auto& a_layer{layer};
            auto a_host{host};
            auto a_port{port};
            async_underlying_handshake(
                a_layer.next_layer(),
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
            auto& a_layer{layer};
            auto a_host{host};
            auto a_path{path};
            a_layer.async_handshake(
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

template <
    typename NextLayer,
    typename CompletionToken
>
auto
async_underlying_handshake(
    bs::websocket::stream<NextLayer>& layer,
    std::string_view host,
    std::string_view port,
    std::string_view path,
    CompletionToken&& token
) {
    return
        as::async_compose<
            CompletionToken,
            void(error_code)
        >(
            ws_handshake_op{
                layer,
                host,
                port,
                path
            },
            token,
            layer
        );
}

template <
    typename NextLayer,
    typename CompletionToken
>
auto
async_underlying_handshake(
    bs::websocket::stream<NextLayer>& layer,
    std::string_view host,
    std::string_view port,
    CompletionToken&& token
) {
    return
        as::async_compose<
            CompletionToken,
            void(error_code)
        >(
            ws_handshake_op{
                layer,
                host,
                port,
                "/"
            },
            token,
            layer
        );
}

} // namespace protocol

} // namespace async_mqtt

#endif // ASYNC_MQTT_PREDEFINED_LAYER_IMPL_WS_HANDSHAKE_HPP
