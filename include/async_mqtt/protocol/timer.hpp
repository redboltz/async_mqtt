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
    pingreq_recv,
    pingresp_recv,
};

constexpr
char const* timer_to_string(timer v);

std::ostream& operator<<(std::ostream& o, timer v);

} // namespace async_mqtt

#if !defined(ASYNC_MQTT_SEPARATE_COMPILATION)
#include <async_mqtt/protocol/impl/timer_impl.ipp>
#endif // !defined(ASYNC_MQTT_SEPARATE_COMPILATION)

#endif // ASYNC_MQTT_PROTOCOL_TIMER_HPP
