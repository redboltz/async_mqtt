// Copyright Takatoshi Kondo 2021
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_BROKER_FIXED_CORE_MAP_HPP)
#define ASYNC_MQTT_BROKER_FIXED_CORE_MAP_HPP

#include <async_mqtt/log.hpp>

#if defined(_GNU_SOURCE)

#include <sched.h>
#include <boost/assert.hpp>

namespace async_mqtt {

inline void map_core_to_this_thread(std::size_t core) {
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(static_cast<int>(core), &mask);
    int ret = sched_setaffinity(0, sizeof(mask), &mask);
    BOOST_ASSERT(ret == 0);
}

} // namespace async_mqtt

#else  // defined(_GNU_SOURCE)

namespace async_mqtt {

inline void map_core_to_this_thread(std::size_t /*core*/) {
    ASYNC_MQTT_LOG("mqtt_broker", warning)
        << "map_core_to_this_thread() is called but do nothing";
}

} // namespace async_mqtt

#endif // defined(_GNU_SOURCE)

#endif // ASYNC_MQTT_BROKER_FIXED_CORE_MAP_HPP
