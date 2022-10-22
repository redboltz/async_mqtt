// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_STREAM_TRAITS_HPP)
#define ASYNC_MQTT_STREAM_TRAITS_HPP

#include <type_traits>

namespace async_mqtt {

template<class T>
using executor_type = decltype(std::declval<T&>().get_executor());

} // namespace async_mqtt

#endif // ASYNC_MQTT_STREAM_TRAITS_HPP
