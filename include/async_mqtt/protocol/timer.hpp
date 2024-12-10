// Copyright Takatoshi Kondo 2024
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PROTOCOL_TIMER_HPP)
#define ASYNC_MQTT_PROTOCOL_TIMER_HPP

#include <ostream>

namespace async_mqtt {

enum class timer_kind {
    pingreq_send,
    pingreq_recv,
    pingresp_recv,
};

enum class timer_op {
    set,
    reset,
    cancel
};

constexpr
char const* timer_kind_to_string(timer_kind v);

std::ostream& operator<<(std::ostream& o, timer_kind v);

constexpr
char const* timer_op_to_string(timer_op v);

std::ostream& operator<<(std::ostream& o, timer_op v);

} // namespace async_mqtt

#if !defined(ASYNC_MQTT_SEPARATE_COMPILATION)
#include <async_mqtt/protocol/impl/timer_impl.ipp>
#endif // !defined(ASYNC_MQTT_SEPARATE_COMPILATION)

#endif // ASYNC_MQTT_PROTOCOL_TIMER_HPP
