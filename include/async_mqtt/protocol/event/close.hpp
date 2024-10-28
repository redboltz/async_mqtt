// Copyright Takatoshi Kondo 2024
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PROTOCOL_EVENT_CLOSE_HPP)
#define ASYNC_MQTT_PROTOCOL_EVENT_CLOSE_HPP

#include <cstddef>

namespace async_mqtt::event {

class close {
public:
    constexpr close() = default;
};

} // namespace async_mqtt::event

#endif // ASYNC_MQTT_PROTOCOL_EVENT_CLOSE_HPP
