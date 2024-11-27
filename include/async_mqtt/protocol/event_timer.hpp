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

namespace async_mqtt {

class event_timer {
public:
    event_timer(
        timer_op op,
        timer_kind kind,
        std::optional<std::chrono::milliseconds> ms = std::nullopt
    )
        :op_{op}, kind_{kind}, ms_{ms}
    {
    }

    timer_op get_op() const {
        return op_;
    }

    timer_kind get_kind() const {
        return kind_;
    }

    std::optional<std::chrono::milliseconds> get_ms() const {
        return ms_;
    }

private:
    timer_op op_;
    timer_kind kind_;
    std::optional<std::chrono::milliseconds> ms_;
};

} // namespace async_mqtt

#endif // ASYNC_MQTT_PROTOCOL_EVENT_TIMER_HPP
