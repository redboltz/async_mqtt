// Copyright Takatoshi Kondo 2024
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PROTOCOL_IMPL_TIMER_IMPL_IPP)
#define ASYNC_MQTT_PROTOCOL_IMPL_TIMER_IMPL_IPP

#include <async_mqtt/protocol/timer.hpp>
#include <async_mqtt/util/inline.hpp>
namespace async_mqtt {

ASYNC_MQTT_HEADER_ONLY_INLINE
constexpr
char const* timer_to_string(timer v) {
    switch (v) {
    case timer::pingreq_send:                 return "pingreq_send";
    case timer::pingreq_recv:                 return "pingreq_recv";
    case timer::pingresp_recv:                return "pingresp_recv";
    default:                                  return "unknown_timer";
    }
}

inline
constexpr
std::ostream& operator<<(std::ostream& o, timer v)
{
    o << timer_to_string(v);
    return o;
}

} // namespace async_mqtt

#endif // ASYNC_MQTT_PROTOCOL_IMPL_TIMER_IMPL_IPP
