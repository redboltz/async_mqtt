// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_IS_STRAND_HPP)
#define ASYNC_MQTT_IS_STRAND_HPP

#include <type_traits>

#include <boost/asio/strand.hpp>
#include <boost/asio/io_context.hpp>

namespace async_mqtt {

namespace as = boost::asio;

template <typename T>
struct is_strand_template : std::false_type {};

template <typename T>
struct is_strand_template<as::strand<T>> : std::true_type {};

template <typename T>
constexpr bool is_strand() {
    return
        is_strand_template<T>::value ||
        std::is_same_v<T, as::io_context::strand>;
}

} // namespace async_mqtt

#endif // ASYNC_MQTT_IS_STRAND_HPP
