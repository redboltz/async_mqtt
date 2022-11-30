// Copyright Takatoshi Kondo 2021
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_BROKER_MUTEX_HPP)
#define ASYNC_MQTT_BROKER_MUTEX_HPP

#include <shared_mutex>

namespace async_mqtt {

using mutex = std::shared_timed_mutex;

} // namespace async_mqtt


#endif // ASYNC_MQTT_BROKER_MUTEX_HPP
