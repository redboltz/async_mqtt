// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PACKET_BUFFER_TO_PACKET_VARIANT_FWD_HPP)
#define ASYNC_MQTT_PACKET_BUFFER_TO_PACKET_VARIANT_FWD_HPP

#include <async_mqtt/protocol_version.hpp>
#include <async_mqtt/util/buffer.hpp>

namespace async_mqtt {

template <std::size_t PacketIdBytes>
class basic_packet_variant;

template <std::size_t PacketIdBytes>
basic_packet_variant<PacketIdBytes> buffer_to_basic_packet_variant(buffer buf, protocol_version ver);

} // namespace async_mqtt

#endif // ASYNC_MQTT_PACKET_BUFFER_TO_PACKET_VARIANT_FWD_HPP
