// Copyright Takatoshi Kondo 2024
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PROTOCOL_EVENT_CLOSE_HPP)
#define ASYNC_MQTT_PROTOCOL_EVENT_CLOSE_HPP

#include <cstddef>

namespace async_mqtt::event {

/**
 * @brief Close event.
 *
 * This corresponds to @ref basic_connection::on_close().
 *
 * #### Thread Safety
 *    @li Distinct objects: Safe
 *    @li Shared objects: Unsafe
 */
class close {
public:
    /**
     * @brief Constructor.
     */
    constexpr close() = default;
};

} // namespace async_mqtt::event

#endif // ASYNC_MQTT_PROTOCOL_EVENT_CLOSE_HPP
