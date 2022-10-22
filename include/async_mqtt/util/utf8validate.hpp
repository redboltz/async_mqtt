// Copyright Takatoshi Kondo 2023
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_UTIL_UTF8VALIDATE_HPP)
#define ASYNC_MQTT_UTIL_UTF8VALIDATE_HPP

#include <string_view>

namespace async_mqtt {

inline bool utf8string_check(std::string_view /*buf*/) {
    return true;
}

} // namespace async_mqtt

#endif // ASYNC_MQTT_UTIL_UTF8VALIDATE_HPP
