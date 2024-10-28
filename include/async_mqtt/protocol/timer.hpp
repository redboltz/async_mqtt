// Copyright Takatoshi Kondo 2024
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PROTOCOL_TIMER_HPP)
#define ASYNC_MQTT_PROTOCOL_TIMER_HPP

#include <ostream>

namespace async_mqtt {

enum class timer {
    pingreq_send,
    pingresp_recv,
};

constexpr
char const* timer_to_string(timer v) {
    switch (v) {
    case timer::pingreq_send:                 return "pingreq_send";
    case timer::pingresp_recv:                return "pingresp_recv";
    default:                                  return "unknown_timer";
    }
}

inline
std::ostream& operator<<(std::ostream& o, timer v)
{
    o << timer_to_string(v);
    return o;
}

} // namespace async_mqtt

#endif // ASYNC_MQTT_PROTOCOL_TIMER_HPP
