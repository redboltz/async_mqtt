// Copyright Takatoshi Kondo 2024
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PROTOCOL_TIMER_HPP)
#define ASYNC_MQTT_PROTOCOL_TIMER_HPP

#include <ostream>

namespace async_mqtt {

/**
 * @brief Types of timers.
 */
enum class timer_kind {
    pingreq_send,  ///< Timer for sending PINGREQ. Set by the client.
    pingreq_recv,  ///< Timer for receiving PINGREQ. Set by the server (broker).
    pingresp_recv, ///< Timer for receiving PINGRESP. Set by the client.
};

/**
 * @brief Types of timer operations.
 */
enum class timer_op {
    reset,  ///< Reset the timer. If a timer is set, it is canceled and then reset; otherwise, a new timer is set.
    cancel  ///< Cancel the timer.
};

/**
 * @brief Convert a timer_kind value to a string.
 *
 * @param v The timer kind.
 * @return The string representation of the given timer kind.
 */
constexpr
char const* timer_kind_to_string(timer_kind v);

/**
 * @brief Output a timer_kind value as a string to the stream.
 *
 * @param o The output stream.
 * @param v The timer kind.
 * @return The output stream.
 */
std::ostream& operator<<(std::ostream& o, timer_kind v);

/**
 * @brief Convert a timer_op value to a string.
 *
 * @param v The timer operation.
 * @return The string representation of the given timer operation.
 */
constexpr
char const* timer_op_to_string(timer_op v);

/**
 * @brief Output a timer_op value as a string to the stream.
 *
 * @param o The output stream.
 * @param v The timer operation.
 * @return The output stream.
 */
std::ostream& operator<<(std::ostream& o, timer_op v);

} // namespace async_mqtt

#if !defined(ASYNC_MQTT_SEPARATE_COMPILATION)
#include <async_mqtt/protocol/impl/timer_impl.ipp>
#endif // !defined(ASYNC_MQTT_SEPARATE_COMPILATION)

#endif // ASYNC_MQTT_PROTOCOL_TIMER_HPP
