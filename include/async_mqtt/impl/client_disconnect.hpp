// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_IMPL_CLIENT_DISCONNECT_HPP)
#define ASYNC_MQTT_IMPL_CLIENT_DISCONNECT_HPP

#include <async_mqtt/client.hpp>
#include <async_mqtt/exception.hpp>
#include <async_mqtt/log.hpp>

namespace async_mqtt {

template <protocol_version Version, typename NextLayer>
struct client<Version, NextLayer>::
disconnect_op {
    this_type& cl;
    disconnect_packet packet;

    template <typename Self>
    void operator()(
        Self& self
    ) {
        auto& a_cl{cl};
        auto a_packet{packet};
        a_cl.ep_->send(
            force_move(a_packet),
            force_move(self)
        );
    }

    template <typename Self>
    void operator()(
        Self& self,
        system_error const& se
    ) {
        self.complete(se.code());
    }
};

template <protocol_version Version, typename NextLayer>
template <typename CompletionToken>
BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(
    CompletionToken,
    void(error_code)
)
client<Version, NextLayer>::disconnect(
    disconnect_packet packet,
    CompletionToken&& token
) {
    ASYNC_MQTT_LOG("mqtt_api", info)
        << ASYNC_MQTT_ADD_VALUE(address, this)
        << packet;
    return
        as::async_compose<
            CompletionToken,
            void(error_code)
        >(
            disconnect_op{
                *this,
                force_move(packet)
            },
            token
        );
}

} // namespace async_mqtt

#endif // ASYNC_MQTT_IMPL_CLIENT_DISCONNECT_HPP
