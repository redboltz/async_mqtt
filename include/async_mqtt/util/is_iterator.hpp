// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_UTIL_IS_ITERATOR_HPP)
#define ASYNC_MQTT_UTIL_IS_ITERATOR_HPP

#include <type_traits>
namespace async_mqtt {

template <typename...>
using void_t = void;

template <typename T, typename = void>
struct is_input_iterator : std::false_type {
};

template <typename T>
struct is_input_iterator<
    T,
    void_t<decltype(++std::declval<T&>()),                       // incrementable,
           decltype(*std::declval<T&>()),                        // dereferencable,
           decltype(std::declval<T&>() == std::declval<T&>())>>  // comparable
    : std::true_type {};

} // namespace async_mqtt

#endif // ASYNC_MQTT_UTIL_IS_ITERATOR_HPP
