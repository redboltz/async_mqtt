// Copyright Takatoshi Kondo 2024
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PROTOCOL_IMPL_RV_CONNECTION_HPP)
#define ASYNC_MQTT_PROTOCOL_IMPL_RV_CONNECTION_HPP

#include <async_mqtt/protocol/rv_connection.hpp>
#include <async_mqtt/util/inline.hpp>

namespace async_mqtt {

// public

template <role Role, std::size_t PacketIdBytes>
template <typename Packet>
inline
std::vector<basic_event_variant<PacketIdBytes>>
basic_rv_connection<Role, PacketIdBytes>::
send(Packet packet) {
    base_type::send(std::forward<Packet>(packet));
    return force_move(events_);
}

} // namespace async_mqtt

#endif // ASYNC_MQTT_PROTOCOL_IMPL_RV_CONNECTION_HPP
