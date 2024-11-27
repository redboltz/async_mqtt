// Copyright Takatoshi Kondo 2024
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PROTOCOL_IMPL_EVENT_CONNECTION_IPP)
#define ASYNC_MQTT_PROTOCOL_IMPL_EVENT_CONNECTION_IPP

#include <async_mqtt/protocol/event_connection.hpp>
#include <async_mqtt/protocol/event_close.hpp>
#include <async_mqtt/protocol/event_timer.hpp>
#include <async_mqtt/protocol/event_packet_id_released.hpp>
#include <async_mqtt/protocol/event_packet_received.hpp>
#include <async_mqtt/util/inline.hpp>

namespace async_mqtt {

// public

template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
basic_event_connection<Role, PacketIdBytes>::
basic_event_connection(protocol_version ver)
    :base_type{ver}
{
}

template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
std::vector<basic_event_variant<PacketIdBytes>>
basic_event_connection<Role, PacketIdBytes>::
recv(std::istream& is) {
    base_type::recv(is);
    return force_move(events_);
}

template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
std::vector<basic_event_variant<PacketIdBytes>>
basic_event_connection<Role, PacketIdBytes>::
notify_timer_fired(timer_kind kind) {
    base_type::notify_timer_fired(kind);
    return force_move(events_);
}

template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
std::vector<basic_event_variant<PacketIdBytes>>
basic_event_connection<Role, PacketIdBytes>::
set_pingreq_send_interval(
    std::chrono::milliseconds duration
) {
    base_type::set_pingreq_send_interval(duration);
    return force_move(events_);
}

template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
std::vector<basic_event_variant<PacketIdBytes>>
basic_event_connection<Role, PacketIdBytes>::
release_packet_id(typename basic_packet_id_type<PacketIdBytes>::type packet_id) {
    base_type::release_packet_id(packet_id);
    return force_move(events_);
}

template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
basic_event_connection<Role, PacketIdBytes>::
on_error(error_code ec) {
    events_.emplace_back(ec);
}

template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
basic_event_connection<Role, PacketIdBytes>::
on_send(
    basic_packet_variant<PacketIdBytes> packet,
    std::optional<typename basic_packet_id_type<PacketIdBytes>::type>
    release_packet_id_if_send_error
) {
    events_.emplace_back(basic_event_send<PacketIdBytes>{force_move(packet), release_packet_id_if_send_error});
}

template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
basic_event_connection<Role, PacketIdBytes>::
on_packet_id_release(
    typename basic_packet_id_type<PacketIdBytes>::type packet_id
) {
    events_.emplace_back(basic_event_packet_id_released<PacketIdBytes>{packet_id});
}

template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
basic_event_connection<Role, PacketIdBytes>::
on_receive(
    basic_packet_variant<PacketIdBytes> packet
) {
    events_.emplace_back(basic_event_packet_received<PacketIdBytes>{force_move(packet)});
}

template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
basic_event_connection<Role, PacketIdBytes>::
on_timer_op(
    timer_op op,
    timer_kind kind,
    std::optional<std::chrono::milliseconds> ms
) {
    events_.emplace_back(event_timer{op, kind, ms});
}

template <role Role, std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
void
basic_event_connection<Role, PacketIdBytes>::
on_close() {
    events_.emplace_back(event_close{});
}

} // namespace async_mqtt

#if defined(ASYNC_MQTT_SEPARATE_COMPILATION)

#include <async_mqtt/detail/instantiate_helper.hpp>


#define ASYNC_MQTT_INSTANTIATE_EACH(a_role, a_size) \
namespace async_mqtt { \
template \
class basic_event_connection_impl<a_role, a_size>; \
template \
class basic_event_connection<a_role, a_size>; \
} // namespace async_mqtt

#define ASYNC_MQTT_PP_GENERATE(r, product) \
    BOOST_PP_EXPAND( \
        ASYNC_MQTT_INSTANTIATE_EACH \
        BOOST_PP_SEQ_TO_TUPLE( \
            product \
        ) \
    )

BOOST_PP_SEQ_FOR_EACH_PRODUCT(ASYNC_MQTT_PP_GENERATE, (ASYNC_MQTT_PP_ROLE)(ASYNC_MQTT_PP_SIZE))

#undef ASYNC_MQTT_PP_GENERATE
#undef ASYNC_MQTT_INSTANTIATE_EACH

#endif // defined(ASYNC_MQTT_SEPARATE_COMPILATION)

#endif // ASYNC_MQTT_PROTOCOL_IMPL_EVENT_CONNECTION_IPP
