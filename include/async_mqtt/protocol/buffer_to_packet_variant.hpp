// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PROTOCOL_BUFFER_TO_PACKET_VARIANT_HPP)
#define ASYNC_MQTT_PROTOCOL_BUFFER_TO_PACKET_VARIANT_HPP

#include <async_mqtt/protocol/error.hpp>
#include <async_mqtt/protocol/protocol_version.hpp>
#include <async_mqtt/protocol/packet/packet_variant_fwd.hpp>
#include <async_mqtt/util/buffer.hpp>

namespace async_mqtt {

/**
 * @brief create basic_packet_variant from the buffer
 * @param buf buffer contains packet bytes
 * @param ver protocol version to create packet
 * @param ec  error_code for reporting error
 * @return if no error created basic_packet_variant, otherwise std::nullopt.
 */
template <std::size_t PacketIdBytes>
std::optional<basic_packet_variant<PacketIdBytes>>
buffer_to_basic_packet_variant(buffer buf, protocol_version ver, error_code& ec);

/**
 * @brief create packet_variant from the buffer
 * @param buf buffer contains packet bytes
 * @param ver protocol version to create packet
 * @param ec  error_code for reporting error
 * @return if no error created packet_variant, otherwise std::nullopt.
 */
std::optional<packet_variant>
buffer_to_packet_variant(buffer buf, protocol_version ver, error_code& ec);

} // namespace async_mqtt

#endif // ASYNC_MQTT_PROTOCOL_BUFFER_TO_PACKET_VARIANT_HPP
