// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_UTIL_OPTIONAL_HPP)
#define ASYNC_MQTT_UTIL_OPTIONAL_HPP

#include <optional>

namespace async_mqtt {

using std::optional;
using std::nullopt_t;
using in_place_t = std::in_place_t;
static constexpr auto nullopt = std::nullopt;

} // namespace async_mqtt

#endif // ASYNC_MQTT_UTIL_OPTIONAL_HPP
