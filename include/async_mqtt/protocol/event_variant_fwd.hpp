// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PROTOCOL_EVENT_VARIANT_FWD_HPP)
#define ASYNC_MQTT_PROTOCOL_EVENT_VARIANT_FWD_HPP

#include <cstddef>

/**
 * @defgroup packet_variant variant class for all packets
 * @ingroup packet
 */

/**
 * @defgroup packet_variant_detail implementation class
 * @ingroup packet_variant
 */

namespace async_mqtt {

/**
 * @ingroup packet_variant_detail
 * @brief The varaint type of all packets and system_error
 *
 * #### Thread Safety
 * @li Distinct objects: Safe
 * @li Shared objects: Unsafe
 *
 * #### variants
 * @li @ref std::monostate
 * @li @ref v3_1_1::connect_packet
 * @li @ref v3_1_1::connack_packet
 * @li @ref v3_1_1::basic_publish_packet<PacketIdBytes>
 * @li @ref v3_1_1::basic_puback_packet<PacketIdBytes>
 * @li @ref v3_1_1::basic_pubrec_packet<PacketIdBytes>
 * @li @ref v3_1_1::basic_pubrel_packet<PacketIdBytes>
 * @li @ref v3_1_1::basic_pubcomp_packet<PacketIdBytes>
 * @li @ref v3_1_1::basic_subscribe_packet<PacketIdBytes>
 * @li @ref v3_1_1::basic_suback_packet<PacketIdBytes>
 * @li @ref v3_1_1::basic_unsubscribe_packet<PacketIdBytes>
 * @li @ref v3_1_1::basic_unsuback_packet<PacketIdBytes>
 * @li @ref v3_1_1::pingreq_packet
 * @li @ref v3_1_1::pingresp_packet
 * @li @ref v3_1_1::disconnect_packet
 * @li @ref v5::connect_packet
 * @li @ref v5::connack_packet
 * @li @ref v5::basic_publish_packet<PacketIdBytes>
 * @li @ref v5::basic_puback_packet<PacketIdBytes>
 * @li @ref v5::basic_pubrec_packet<PacketIdBytes>
 * @li @ref v5::basic_pubrel_packet<PacketIdBytes>
 * @li @ref v5::basic_pubcomp_packet<PacketIdBytes>
 * @li @ref v5::basic_subscribe_packet<PacketIdBytes>
 * @li @ref v5::basic_suback_packet<PacketIdBytes>
 * @li @ref v5::basic_unsubscribe_packet<PacketIdBytes>
 * @li @ref v5::basic_unsuback_packet<PacketIdBytes>
 * @li @ref v5::pingreq_packet
 * @li @ref v5::pingresp_packet
 * @li @ref v5::disconnect_packet
 * @li @ref v5::auth_packet
 *
 * #### Requirements
 * @li Header: async_mqtt/packet/packet_variant.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
template <std::size_t PacketIdBytes>
class basic_event_variant;

/**
 * @ingroup event_variant
 * @related basic_event_variant
 * @brief type alias of basic_event_variant (PacketIdBytes=2).
 *
 * #### Requirements
 * @li Header: async_mqtt/packet/packet_variant.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
using event_variant = basic_event_variant<2>;

} // namespace async_mqtt

#endif // ASYNC_MQTT_PROTOCOL_EVENT_VARIANT_FWD_HPP
