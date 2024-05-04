// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_UTIL_SHARED_PTR_ARRAY_HPP)
#define ASYNC_MQTT_UTIL_SHARED_PTR_ARRAY_HPP

#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/smart_ptr/make_shared.hpp>

#include <async_mqtt/util/allocator.hpp>

namespace async_mqtt {

namespace as = boost::asio;

/**
 * @brief Type alias of shared_ptr char array.
 */
using shared_ptr_array = boost::shared_ptr<char []>;
using const_shared_ptr_array = boost::shared_ptr<char const []>;

/**
 * @brief shared_ptr_array creating function.
 */
inline shared_ptr_array make_shared_ptr_array(std::size_t size) {
    return boost::allocate_shared<char[]>(allocator<char[]>(), size);
}

} // namespace async_mqtt

#endif // ASYNC_MQTT_UTIL_SHARED_PTR_ARRAY_HPP
