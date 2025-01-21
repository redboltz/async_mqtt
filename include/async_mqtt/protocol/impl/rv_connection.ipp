// Copyright Takatoshi Kondo 2024
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PROTOCOL_IMPL_RV_CONNECTION_IPP)
#define ASYNC_MQTT_PROTOCOL_IMPL_RV_CONNECTION_IPP

#include <async_mqtt/protocol/rv_connection.hpp>
#include <async_mqtt/protocol/event/close.hpp>
#include <async_mqtt/protocol/event/send.hpp>
#include <async_mqtt/protocol/event/timer.hpp>
#include <async_mqtt/protocol/event/packet_id_released.hpp>
#include <async_mqtt/protocol/event/packet_received.hpp>
#include <async_mqtt/util/inline.hpp>

namespace async_mqtt {

// public

template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
basic_rv_connection<Role, PacketIdBytes>::
basic_rv_connection(protocol_version ver)
    :base_type{ver}
{
}

template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
std::vector<basic_event_variant<PacketIdBytes>>
basic_rv_connection<Role, PacketIdBytes>::
recv(std::istream& is) {
    base_type::recv(is);
    return force_move(events_);
}

template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
std::vector<basic_event_variant<PacketIdBytes>>
basic_rv_connection<Role, PacketIdBytes>::
notify_timer_fired(timer_kind kind) {
    base_type::notify_timer_fired(kind);
    return force_move(events_);
}

template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
std::vector<basic_event_variant<PacketIdBytes>>
basic_rv_connection<Role, PacketIdBytes>::
notify_closed() {
    base_type::notify_closed();
    return force_move(events_);
}

template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
std::vector<basic_event_variant<PacketIdBytes>>
basic_rv_connection<Role, PacketIdBytes>::
set_pingreq_send_interval(
    std::chrono::milliseconds duration
) {
    base_type::set_pingreq_send_interval(duration);
    return force_move(events_);
}

template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
std::vector<basic_event_variant<PacketIdBytes>>
basic_rv_connection<Role, PacketIdBytes>::
release_packet_id(typename basic_packet_id_type<PacketIdBytes>::type packet_id) {
    base_type::release_packet_id(packet_id);
    return force_move(events_);
}

template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
basic_rv_connection<Role, PacketIdBytes>::
on_error(error_code ec) {
    events_.emplace_back(ec);
}

template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
basic_rv_connection<Role, PacketIdBytes>::
on_send(
    basic_packet_variant<PacketIdBytes> packet,
    std::optional<typename basic_packet_id_type<PacketIdBytes>::type>
    release_packet_id_if_send_error
) {
    events_.emplace_back(event::basic_send<PacketIdBytes>{force_move(packet), release_packet_id_if_send_error});
}

template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
basic_rv_connection<Role, PacketIdBytes>::
on_packet_id_release(
    typename basic_packet_id_type<PacketIdBytes>::type packet_id
) {
    events_.emplace_back(event::basic_packet_id_released<PacketIdBytes>{packet_id});
}

template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
basic_rv_connection<Role, PacketIdBytes>::
on_receive(
    basic_packet_variant<PacketIdBytes> packet
) {
    events_.emplace_back(event::basic_packet_received<PacketIdBytes>{force_move(packet)});
}

template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
basic_rv_connection<Role, PacketIdBytes>::
on_timer_op(
    timer_op op,
    timer_kind kind,
    std::optional<std::chrono::milliseconds> ms
) {
    events_.emplace_back(event::timer{op, kind, ms});
}

template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
basic_rv_connection<Role, PacketIdBytes>::
on_close() {
    events_.emplace_back(event::close{});
}

} // namespace async_mqtt

#include <async_mqtt/protocol/impl/rv_connection_instantiate.hpp>

#endif // ASYNC_MQTT_PROTOCOL_IMPL_RV_CONNECTION_IPP
