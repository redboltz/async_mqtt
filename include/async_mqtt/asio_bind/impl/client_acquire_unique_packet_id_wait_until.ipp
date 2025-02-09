// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_IMPL_CLIENT_ACQUIRE_UNIQUE_PACKET_ID_WAIT_UNTIL_IPP)
#define ASYNC_MQTT_IMPL_CLIENT_ACQUIRE_UNIQUE_PACKET_ID_WAIT_UNTIL_IPP

#include <async_mqtt/client.hpp>
#include <async_mqtt/asio_bind/impl/client_impl.hpp>
#include <async_mqtt/util/inline.hpp>

namespace async_mqtt::detail {

template <protocol_version Version, typename NextLayer>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
client_impl<Version, NextLayer>::async_acquire_unique_packet_id_wait_until(
    this_type_sp impl,
    as::any_completion_handler<
        void(error_code, packet_id_type)
    > handler
) {
    impl->ep_.async_acquire_unique_packet_id_wait_until(force_move(handler));
}

} // namespace async_mqtt::detail

#include <async_mqtt/asio_bind/impl/client_instantiate.hpp>

#endif // ASYNC_MQTT_IMPL_CLIENT_ACQUIRE_UNIQUE_PACKET_ID_WAIT_UNTIL_IPP
