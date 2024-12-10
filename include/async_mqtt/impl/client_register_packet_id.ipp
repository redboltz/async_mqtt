// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_IMPL_CLIENT_REGISTER_PACKET_ID_IPP)
#define ASYNC_MQTT_IMPL_CLIENT_REGISTER_PACKET_ID_IPP

#include <async_mqtt/client.hpp>
#include <async_mqtt/impl/client_impl.hpp>
#include <async_mqtt/util/inline.hpp>

namespace async_mqtt::detail {

template <protocol_version Version, typename NextLayer>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
client_impl<Version, NextLayer>::async_register_packet_id(
    this_type_sp impl,
    packet_id_type pid,
    as::any_completion_handler<
        void(error_code)
    > handler
) {
    impl->ep_.async_register_packet_id(pid, force_move(handler));
}

// sync version

template <protocol_version Version, typename NextLayer>
inline
bool
client_impl<Version, NextLayer>::register_packet_id(packet_id_type packet_id) {
    return ep_.register_packet_id(packet_id);
}

} // namespace async_mqtt::detail

#include <async_mqtt/impl/client_instantiate.hpp>

#endif // ASYNC_MQTT_IMPL_CLIENT_REGISTER_PACKET_ID_IPP
