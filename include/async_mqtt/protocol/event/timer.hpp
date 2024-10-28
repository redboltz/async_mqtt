// Copyright Takatoshi Kondo 2024
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PROTOCOL_EVENT_TIMER_HPP)
#define ASYNC_MQTT_PROTOCOL_EVENT_TIMER_HPP

#include <cstddef>
#include <chrono>
#include <optional>

#include <async_mqtt/protocol/timer.hpp>

namespace async_mqtt::event {

/**
 * @brief Timer event.
 *
 * This corresponds to @ref basic_connection::on_timer_op().
 *
 * #### Thread Safety
 *    @li Distinct objects: Safe
 *    @li Shared objects: Unsafe
 */
class timer {
public:
    /**
     * @brief Constructor.
     *
     * @param op   The type of timer operation.
     * @param kind The type of timer.
     * @param ms   The duration of the timer.
     *             The minimum resolution is in milliseconds.
     *             If `kind` is @ref timer_op::cancel, `ms` is ignored.
     */
    explicit timer(
        timer_op op,
        timer_kind kind,
        std::optional<std::chrono::milliseconds> ms = std::nullopt
    )
        : op_{op}, kind_{kind}, ms_{ms} {}

    /**
     * @brief Get the timer operation.
     *
     * @return The timer operation.
     */
    timer_op get_op() const {
        return op_;
    }

    /**
     * @brief Get the timer type.
     *
     * @return The timer type.
     */
    timer_kind get_kind() const {
        return kind_;
    }

    /**
     * @brief Get the timer duration in milliseconds.
     *
     * @return The timer duration in milliseconds.
     */
    std::optional<std::chrono::milliseconds> get_ms() const {
        return ms_;
    }

private:
    timer_op op_;
    timer_kind kind_;
    std::optional<std::chrono::milliseconds> ms_;
};

} // namespace async_mqtt::event

#endif // ASYNC_MQTT_PROTOCOL_EVENT_TIMER_HPP
