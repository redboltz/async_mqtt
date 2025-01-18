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
char const* timer_kind_to_string(timer_kind v) {
    switch (v) {
    case timer_kind::pingreq_send:  return "pingreq_send";
    case timer_kind::pingreq_recv:  return "pingreq_recv";
    case timer_kind::pingresp_recv: return "pingresp_recv";
    default:                        return "unknown_timer_kind";
    }
}

inline
std::ostream& operator<<(std::ostream& o, timer_kind v)
{
    o << timer_kind_to_string(v);
    return o;
}

constexpr
char const* event_timer_op_to_string(timer_op const& v) {
    switch (v) {
    case timer_op::set:    return "set";
    case timer_op::reset:  return "reset";
    case timer_op::cancel: return "cancel";
    default:               return "unknown_timer_op";
    }
}

inline
std::ostream& operator<<(std::ostream& o, timer_op v)
{
    o << event_timer_op_to_string(v);
    return o;
}

} // namespace async_mqtt

#endif // ASYNC_MQTT_PROTOCOL_IMPL_TIMER_IMPL_IPP
