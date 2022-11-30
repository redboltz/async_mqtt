// Copyright Takatoshi Kondo 2020
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_TIME_POINT_HPP)
#define ASYNC_MQTT_TIME_POINT_HPP

#include <chrono>

namespace async_mqtt {

using time_point_t = std::chrono::time_point<std::chrono::steady_clock>;

} // namespace async_mqtt

#endif // ASYNC_MQTT_TIME_POINT_HPP
