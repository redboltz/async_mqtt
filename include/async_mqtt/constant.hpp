// Copyright Takatoshi Kondo 2020
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_CONSTANT_HPP)
#define ASYNC_MQTT_CONSTANT_HPP

#include <cstddef>
#include <chrono>

#include <async_mqtt/type.hpp>

namespace async_mqtt {

static constexpr session_expiry_interval_t session_never_expire = 0xffffffffUL;
static constexpr topic_alias_t topic_alias_max = 0xffff;
static constexpr std::uint32_t packet_size_no_limit =
    1 + // fixed header
    4 + // remaining length
    128 * 128 * 128 * 128; // maximum value of remainin length
static constexpr receive_maximum_t receive_maximum_max = 0xffff;
static constexpr auto shutdown_timeout = std::chrono::seconds(3);

} // namespace async_mqtt

#endif // ASYNC_MQTT_CONSTANT_HPP
