// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PACKET_FIXED_HEADER_HPP)
#define ASYNC_MQTT_PACKET_FIXED_HEADER_HPP

#include <async_mqtt/packet/control_packet_type.hpp>

namespace async_mqtt {

constexpr std::uint8_t make_fixed_header(control_packet_type type, std::uint8_t flags) {
    return static_cast<std::uint8_t>(type) | (flags & 0x0f);
}

} // namespace async_mqtt

#endif // ASYNC_MQTT_PACKET_FIXED_HEADER_HPP
