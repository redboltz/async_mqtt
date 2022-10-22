// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_UTIL_STATIC_VECTOR_HPP)
#define ASYNC_MQTT_UTIL_STATIC_VECTOR_HPP

#include <boost/container/static_vector.hpp>

namespace async_mqtt {

template <typename T, std::size_t size>
using static_vector = boost::container::static_vector<T, size>;

} // namespace async_mqtt

#endif // ASYNC_MQTT_UTIL_STATIC_VECTOR_HPP
