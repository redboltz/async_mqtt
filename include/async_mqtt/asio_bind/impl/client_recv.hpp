// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_ASIO_BIND_IMPL_CLIENT_RECV_HPP)
#define ASYNC_MQTT_ASIO_BIND_IMPL_CLIENT_RECV_HPP

#include <boost/asio/dispatch.hpp>

#include <async_mqtt/asio_bind/client.hpp>
#include <async_mqtt/asio_bind/impl/client_impl.hpp>
#include <async_mqtt/protocol/packet/packet_variant.hpp>
#include <async_mqtt/util/log.hpp>

namespace async_mqtt {

template <protocol_version Version, typename NextLayer>
template <typename CompletionToken>
auto
client<Version, NextLayer>::async_recv(
    CompletionToken&& token
) {
    ASYNC_MQTT_LOG("mqtt_api", info)
        << ASYNC_MQTT_ADD_VALUE(address, this)
        << "recv";
    BOOST_ASSERT(impl_);
    return
        as::async_initiate<
            CompletionToken,
            void(error_code, std::optional<packet_variant>)
        >(
            [](
                auto handler,
                std::shared_ptr<impl_type> impl
            ) {
                impl_type::async_recv(
                    force_move(impl),
                    force_move(handler)
                );
            },
            token,
            impl_
        );
}

} // namespace async_mqtt

#if !defined(ASYNC_MQTT_SEPARATE_COMPILATION)
#include <async_mqtt/asio_bind/impl/client_recv.ipp>
#endif // !defined(ASYNC_MQTT_SEPARATE_COMPILATION)

#endif // ASYNC_MQTT_ASIO_BIND_IMPL_CLIENT_RECV_HPP
