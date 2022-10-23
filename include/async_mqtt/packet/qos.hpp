// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PACKET_QOS_HPP)
#define ASYNC_MQTT_PACKET_QOS_HPP

#include <cstdint>
#include <ostream>

namespace async_mqtt {

enum class qos : std::uint8_t
{
    at_most_once = 0b00000000,
    at_least_once = 0b00000001,
    exactly_once = 0b00000010,
};

constexpr char const* qos_to_str(qos v) {
    switch(v) {
    case qos::at_most_once:  return "at_most_once";
    case qos::at_least_once: return "at_least_once";
    case qos::exactly_once:  return "exactly_once";
    default:                 return "invalid_qos";
    }
}

inline
std::ostream& operator<<(std::ostream& os, qos val)
{
    os << qos_to_str(val);
    return os;
}

} // namespace async_mqtt

#endif // ASYNC_MQTT_PACKET_QOS_HPP
