// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_ASIO_BIND_IMPL_CLIENT_ACQUIRE_UNIQUE_PACKET_ID_HPP)
#define ASYNC_MQTT_ASIO_BIND_IMPL_CLIENT_ACQUIRE_UNIQUE_PACKET_ID_HPP

#include <async_mqtt/asio_bind/client.hpp>
#include <async_mqtt/asio_bind/impl/client_impl.hpp>

namespace async_mqtt {

template <protocol_version Version, typename NextLayer>
template <typename CompletionToken>
auto
client<Version, NextLayer>::async_acquire_unique_packet_id(
    CompletionToken&& token
) {
    BOOST_ASSERT(impl_);
    return
        as::async_initiate<
            CompletionToken,
            void(error_code, packet_id_type)
        >(
            [](
                auto handler,
                std::shared_ptr<impl_type> impl
            ) {
                impl_type::async_acquire_unique_packet_id(
                    force_move(impl),
                    force_move(handler)
                );
            },
            token,
            impl_
        );
}

// sync version

template <protocol_version Version, typename NextLayer>
inline
std::optional<packet_id_type>
client<Version, NextLayer>::acquire_unique_packet_id() {
    BOOST_ASSERT(impl_);
    return impl_->acquire_unique_packet_id();
}

} // namespace async_mqtt

#if !defined(ASYNC_MQTT_SEPARATE_COMPILATION)
#include <async_mqtt/asio_bind/impl/client_acquire_unique_packet_id.ipp>
#endif // !defined(ASYNC_MQTT_SEPARATE_COMPILATION)

#endif // ASYNC_MQTT_ASIO_BIND_IMPL_CLIENT_ACQUIRE_UNIQUE_PACKET_ID_HPP
