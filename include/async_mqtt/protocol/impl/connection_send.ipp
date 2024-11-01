// Copyright Takatoshi Kondo 2024
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PROTOCOL_IMPL_CONNECTION_SEND_IPP)
#define ASYNC_MQTT_PROTOCOL_IMPL_CONNECTION_SEND_IPP

#include <async_mqtt/protocol/connection.hpp>
#include <async_mqtt/util/inline.hpp>

namespace async_mqtt::detail {

template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
basic_connection_impl<Role, PacketIdBytes>::
send_stored(std::vector<basic_event_variant<PacketIdBytes>>& events) {
    store_.for_each(
        [&](basic_store_packet_variant<PacketIdBytes> const& pv) mutable {
            if (pv.size() > maximum_packet_size_send_) {
                pid_man_.release_id(pv.packet_id());
                // TBD some event should be pushed (size error not send id reusable)
                // Or perhaps nothing is required
                return false;
            }
            pv.visit(
                // copy packet because the stored packets need to be preserved
                // until receiving puback/pubrec/pubcomp
                [&](auto const& p) {
                    events.emplace_back(event_send{p});
                }
            );
            return true;
        }
   );
}

template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
constexpr bool
basic_connection_impl<Role, PacketIdBytes>::can_send_as_client(role r) {
    return
        static_cast<int>(r) &
        static_cast<int>(role::client);
}

template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
constexpr bool
basic_connection_impl<Role, PacketIdBytes>::can_send_as_server(role r) {
    return
        static_cast<int>(r) &
        static_cast<int>(role::server);
}

} // namespace async_mqtt::detail


#endif // ASYNC_MQTT_PROTOCOL_IMPL_CONNECTION_SEND_IPP
