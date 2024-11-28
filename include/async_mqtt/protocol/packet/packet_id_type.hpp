// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PROTOCOL_PACKET_PACKET_ID_TYPE_HPP)
#define ASYNC_MQTT_PROTOCOL_PACKET_PACKET_ID_TYPE_HPP

#include <cstdint>
#include <cstddef>

namespace async_mqtt {

/**
 * @ingroup packet
 * @brief packet idenfitifer type class template
 *
 * #### Actual Types
 * @li `std::uint16_t` if PacketIdBytes is 2. For MQTT specification.
 * @li `std::uint32_t` if PacketIdBytes is 4. For expanded specification for inter broker communication.
 *
 * #### Requirements
 * @li Header: async_mqtt/protocol/packet/packet_id_type.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
template <std::size_t PacketIdBytes>
struct basic_packet_id_type;

template <>
struct basic_packet_id_type<2> {
    using type = std::uint16_t;
};

template <>
struct basic_packet_id_type<4> {
    using type = std::uint32_t;
};

/**
 * @ingroup packet
 * @brief packet idenfitifer type
 *
 * #### Requirements
 * @li Header: async_mqtt/protocol/packet/packet_id_type.hpp
 * @li Convenience header: async_mqtt/all.hpp
 *
 */
using packet_id_type = typename basic_packet_id_type<2>::type;

} // namespace async_mqtt

#endif // ASYNC_MQTT_PROTOCOL_PACKET_PACKET_ID_TYPE_HPP
