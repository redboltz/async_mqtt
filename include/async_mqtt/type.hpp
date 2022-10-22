// Copyright Takatoshi Kondo 2020
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_TYPE_HPP)
#define ASYNC_MQTT_TYPE_HPP

#include <cstdint>

namespace async_mqtt {

using session_expiry_interval_t = std::uint32_t;
using topic_alias_t = std::uint16_t;
using receive_maximum_t = std::uint16_t;

} // namespace async_mqtt

#endif // ASYNC_MQTT_TYPE_HPP
