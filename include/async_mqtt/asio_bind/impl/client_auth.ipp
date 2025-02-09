// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_IMPL_CLIENT_AUTH_IPP)
#define ASYNC_MQTT_IMPL_CLIENT_AUTH_IPP

#include <async_mqtt/client.hpp>
#include <async_mqtt/asio_bind/impl/client_impl.hpp>
#include <async_mqtt/util/log.hpp>
#include <async_mqtt/protocol/packet/v5_auth.hpp>
#include <async_mqtt/util/inline.hpp>

namespace async_mqtt::detail {

template <protocol_version Version, typename NextLayer>
struct client_impl<Version, NextLayer>::
auth_op {
    this_type_sp cl;
    error_code ec;
    std::optional<typename client_type::auth_packet> packet;

    template <typename Self>
    void operator()(
        Self& self
    ) {
        auto& a_cl{*cl};
        if (ec) {
            self.complete(ec);
            return;
        }
        auto a_packet{force_move(*packet)};
        a_cl.ep_.async_send(
            force_move(a_packet),
            force_move(self)
        );
    }

    template <typename Self>
    void operator()(
        Self& self,
        error_code const& ec
    ) {
        self.complete(ec);
    }
};

template <protocol_version Version, typename NextLayer>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
client_impl<Version, NextLayer>::async_auth_impl(
    this_type_sp impl,
    error_code ec,
    std::optional<typename client_type::auth_packet> packet,
    as::any_completion_handler<
        void(error_code)
    > handler
) {
    BOOST_ASSERT(impl);
    if (packet) {
        ASYNC_MQTT_LOG("mqtt_api", info)
            << ASYNC_MQTT_ADD_VALUE(address, impl.get())
            << *packet;
    }
    auto exe = impl->get_executor();
    as::async_compose<
        as::any_completion_handler<
            void(error_code)
        >,
        void(error_code)
    >(
        auth_op{
            force_move(impl),
            ec,
            force_move(packet)
        },
        handler,
        exe
    );
}

} // namespace async_mqtt::detail

#include <async_mqtt/asio_bind/impl/client_instantiate.hpp>

#endif // ASYNC_MQTT_IMPL_CLIENT_AUTH_IPP
