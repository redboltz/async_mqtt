// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_IMPL_CLIENT_RELEASE_PACKET_ID_IPP)
#define ASYNC_MQTT_IMPL_CLIENT_RELEASE_PACKET_ID_IPP

#include <async_mqtt/client.hpp>
#include <async_mqtt/asio_bind/impl/client_impl.hpp>
#include <async_mqtt/util/inline.hpp>

namespace async_mqtt::detail {

template <protocol_version Version, typename NextLayer>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
client_impl<Version, NextLayer>::async_release_packet_id(
    this_type_sp impl,
    packet_id_type pid,
    as::any_completion_handler<
        void()
    > handler
) {
    impl->ep_.async_release_packet_id(pid, force_move(handler));
}

// sync version

template <protocol_version Version, typename NextLayer>
inline
void
client_impl<Version, NextLayer>::release_packet_id(packet_id_type packet_id) {
    ep_.release_packet_id(packet_id);
}

} // namespace async_mqtt::detail

#include <async_mqtt/asio_bind/impl/client_instantiate.hpp>

#endif // ASYNC_MQTT_IMPL_CLIENT_RELEASE_PACKET_ID_IPP
