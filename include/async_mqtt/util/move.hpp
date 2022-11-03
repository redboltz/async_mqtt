// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_UTIL_MOVE_HPP)
#define ASYNC_MQTT_UTIL_MOVE_HPP

#include <utility>
#include <type_traits>

namespace async_mqtt {

template <typename T>
constexpr
typename std::remove_reference_t<T>&&
force_move(T&& t) {
    static_assert(!std::is_const<std::remove_reference_t<T>>::value, "T is const. Fallback to copy.");
    return std::move(t);
}

} // namespace async_mqtt

#endif // ASYNC_MQTT_UTIL_MOVE_HPP
