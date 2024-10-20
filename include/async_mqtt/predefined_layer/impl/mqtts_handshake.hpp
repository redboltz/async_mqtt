// Copyright Takatoshi Kondo 2024
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PREDEFINED_LAYER_IMPL_MQTTS_HANDSHAKE_HPP)
#define ASYNC_MQTT_PREDEFINED_LAYER_IMPL_MQTTS_HANDSHAKE_HPP

#include <async_mqtt/predefined_layer/mqtts.hpp>

namespace async_mqtt {
namespace as = boost::asio;

template <typename NextLayer>
struct mqtts_handshake_op {
    mqtts_handshake_op(
        as::ssl::stream<NextLayer>& layer,
        std::string_view host,
        std::string_view port
    ):layer{layer},
      host{host},
      port{port}
    {}

    as::ssl::stream<NextLayer>& layer;
    std::string host;
    std::string port;
    enum { dispatch, under, handshake, complete } state = dispatch;

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
            a_layer.async_handshake(
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
    typename NextLayer,
    typename CompletionToken
>
auto
async_underlying_handshake(
    as::ssl::stream<NextLayer>& layer,
    std::string_view host,
    std::string_view port,
    CompletionToken&& token
) {
    return
        as::async_compose<
            CompletionToken,
            void(error_code)
        >(
            mqtts_handshake_op{
                layer,
                host,
                port
            },
            token,
            layer
        );
}

} // namespace async_mqtt

#endif // ASYNC_MQTT_PREDEFINED_LAYER_IMPL_MQTTS_HANDSHAKE_HPP
