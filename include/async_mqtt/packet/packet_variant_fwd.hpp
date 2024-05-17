// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PACKET_PACKET_VARIANT_FWD_HPP)
#define ASYNC_MQTT_PACKET_PACKET_VARIANT_FWD_HPP

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
 */
template <std::size_t PacketIdBytes>
class basic_packet_variant;

/**
 * @ingroup packet_variant
 * @related basic_packet_variant
 * @brief type alias of basic_packet_variant (PacketIdBytes=2).
 */
using packet_variant = basic_packet_variant<2>;

} // namespace async_mqtt

#endif // ASYNC_MQTT_PACKET_PACKET_VARIANT_FWD_HPP
