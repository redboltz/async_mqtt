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
    enum class op_type {
        set,
        reset,
        cancel
    };
    event_timer(
        op_type op,
        timer timer_for,
        std::optional<std::chrono::milliseconds> ms = std::nullopt
    )
        :op_{op}, timer_for_{timer_for}, ms_{ms}
    {
    }

    op_type get_op() const {
        return op_;
    }

    timer get_timer_for() const {
        return timer_for_;
    }

    std::optional<std::chrono::milliseconds> get_ms() const {
        return ms_;
    }

private:
    op_type op_;
    timer timer_for_;
    std::optional<std::chrono::milliseconds> ms_;
};

constexpr
char const* event_timer_op_type_to_string(event_timer::op_type const& v) {
    switch (v) {
    case event_timer::op_type::set:        return "set";
    case event_timer::op_type::reset:      return "reset";
    case event_timer::op_type::cancel:     return "cancel";
    default:                               return "unknown_event_timer";
    }
}

inline
std::ostream& operator<<(std::ostream& o, event_timer::op_type v)
{
    o << event_timer_op_type_to_string(v);
    return o;
}

} // namespace async_mqtt

#endif // ASYNC_MQTT_PROTOCOL_EVENT_TIMER_HPP
