// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_FILTER_HPP)
#define ASYNC_MQTT_FILTER_HPP

namespace async_mqtt {

/**
 * @brief receive packet filter
 */
enum class filter {
    match,  ///< matched control_packet_type is target
    except  ///< no matched control_packet_type is target
};

} // namespace async_mqtt

#endif // ASYNC_MQTT_FILTER_HPP
