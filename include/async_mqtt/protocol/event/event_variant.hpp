// Copyright Takatoshi Kondo 2024
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PROTOCOL_EVENT_VARIANT_HPP)
#define ASYNC_MQTT_PROTOCOL_EVENT_VARIANT_HPP

#include <variant>

#include <async_mqtt/protocol/error.hpp>
#include <async_mqtt/protocol/event/send.hpp>
#include <async_mqtt/protocol/event/packet_id_released.hpp>
#include <async_mqtt/protocol/event/packet_received.hpp>
#include <async_mqtt/protocol/event/timer.hpp>
#include <async_mqtt/protocol/event/close.hpp>

namespace async_mqtt {

/**
 * @brief The varaint type of all packets and system_error
 *
 * #### Thread Safety
 * @li Distinct objects: Safe
 * @li Shared objects: Unsafe
 *
 * #### variants
 * @li @ref error_code,
 * @li @ref event::basic_send<PacketIdBytes>
 * @li @ref event::basic_packet_id_released<PacketIdBytes>
 * @li @ref event::basic_packet_received<PacketIdBytes>
 * @li @ref event::timer
 * @li @ref event::close
 *
 */
template <std::size_t PacketIdBytes>
using basic_event_variant = std::variant<
    error_code,
    event::basic_send<PacketIdBytes>,
    event::basic_packet_id_released<PacketIdBytes>,
    event::basic_packet_received<PacketIdBytes>,
    event::timer,
    event::close
>;

/**
 * @ingroup event_variant
 * @related basic_event_variant
 * @brief type alias of basic_event_variant (PacketIdBytes=2).
 *
 */
using event_variant = basic_event_variant<2>;

template <std::size_t PacketIdBytes>
inline bool is_error(basic_event_variant<PacketIdBytes> const& ev) {
    return ev.index() == 0;
}

} // namespace async_mqtt

#endif // ASYNC_MQTT_PROTOCOL_EVENT_VARIANT_HPP
