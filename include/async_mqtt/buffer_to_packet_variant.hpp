// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_BUFFER_TO_PACKET_VARIANT_HPP)
#define ASYNC_MQTT_BUFFER_TO_PACKET_VARIANT_HPP

#include <async_mqtt/error.hpp>
#include <async_mqtt/protocol_version.hpp>
#include <async_mqtt/packet/packet_variant_fwd.hpp>
#include <async_mqtt/util/buffer.hpp>

namespace async_mqtt {

/**
 * @ingroup packet_variant
 * @brief create basic_packet_variant from the buffer
 * @param buf buffer contains packet bytes
 * @param ver protocol version to create packet
 * @param ec  error_code for reporting error
 * @return created basic_packet_variant
 */
template <std::size_t PacketIdBytes>
basic_packet_variant<PacketIdBytes> buffer_to_basic_packet_variant(buffer buf, protocol_version ver, error_code& ec);

/**
 * @ingroup packet_variant
 * @brief create packet_variant from the buffer
 * @param buf buffer contains packet bytes
 * @param ver protocol version to create packet
 * @param ec  error_code for reporting error
 * @return created packet_variant
 */
packet_variant buffer_to_packet_variant(buffer buf, protocol_version ver, error_code& ec);

} // namespace async_mqtt

#endif // ASYNC_MQTT_BUFFER_TO_PACKET_VARIANT_HPP
