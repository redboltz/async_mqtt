// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PROTOCOL_PACKET_QOS_HPP)
#define ASYNC_MQTT_PROTOCOL_PACKET_QOS_HPP

#include <cstdint>
#include <ostream>

namespace async_mqtt {

/**
 * @brief MQTT QoS
 *
 * \n See <a href="https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901234"></a>
 *
 */
enum class qos : std::uint8_t
{
    at_most_once = 0b00000000,  ///< At most once delivery.
    at_least_once = 0b00000001, ///< At least once delivery.
    exactly_once = 0b00000010,  ///< Exactly once delivery.
};

/**
 * @brief stringize qos
 * @param v target
 * @return QoS string
 *
 */
constexpr char const* qos_to_str(qos v) {
    switch(v) {
    case qos::at_most_once:  return "at_most_once";
    case qos::at_least_once: return "at_least_once";
    case qos::exactly_once:  return "exactly_once";
    default:                 return "invalid_qos";
    }
}

/**
 * @brief output to the stream
 * @param o output stream
 * @param v  target
 * @return output stream
 *
 */
inline
std::ostream& operator<<(std::ostream& o, qos v)
{
    o << qos_to_str(v);
    return o;
}

} // namespace async_mqtt

#endif // ASYNC_MQTT_PROTOCOL_PACKET_QOS_HPP
