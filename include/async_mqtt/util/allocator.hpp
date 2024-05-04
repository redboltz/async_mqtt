// Copyright Takatoshi Kondo 2024
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_UTIL_ALLOCATOR_HPP)
#define ASYNC_MQTT_UTIL_ALLOCATOR_HPP

#include <boost/asio/recycling_allocator.hpp>

namespace async_mqtt {

namespace as = boost::asio;

/**
 * @brief Type alias of shared_ptr char array.
 */
template <typename T>
using allocator = as::recycling_allocator<T>;

} // namespace async_mqtt

#endif // ASYNC_MQTT_UTIL_ALLOCATOR_HPP
