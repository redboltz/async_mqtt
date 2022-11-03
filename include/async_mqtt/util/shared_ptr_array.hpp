// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_UTIL_SHARED_PTR_ARRAY_HPP)
#define ASYNC_MQTT_UTIL_SHARED_PTR_ARRAY_HPP

#if defined(_DOXYGEN_)

/**
 * @brief Type alias of shared_ptr char array.
 * You can choose the target type.
 * - If ASYNC_MQTT_STD_SHARED_PTR_ARRAY is defined, `std::shared_ptr<char []>` is used.
 *   - std::shared_ptr<char []> is supported since C++17.
 * - If ASYNC_MQTT_STD_SHARED_PTR_ARRAY is not defined (default), `boost::shared_ptr<char []>` is used.
 *   - `boost::shared_ptr<char []>` can be used on C++14.
 */
using shared_ptr_array = std::shared_ptr<char []>;
using const_shared_ptr_array = std::shared_ptr<char const []>;

/**
 * @brief shared_ptr_array creating function.
 * You can choose the target type.
 * - If ASYNC_MQTT_STD_SHARED_PTR_ARRAY is defined,
 *   - and if your compiler setting is C++20 or later, then `std::make_shared<char[]>(size)` is used.
 *      - It can allocate an array of characters and the control block in a single allocation.
 *   - otherwise `std::shared_ptr<char[]>(new char[size])` is used.
 *      - It requires two times allocations.
 * - If ASYNC_MQTT_STD_SHARED_PTR_ARRAY is not defined (default), then `boost::make_shared<char[]>(size)` is used.
 *      - It can allocate an array of characters and the control block in a single allocation.
 */
inline shared_ptr_array make_shared_ptr_array(std::size_t size);

#else  // defined(_DOXYGEN_)


#include <memory>

namespace async_mqtt {

using shared_ptr_array = std::shared_ptr<char []>;
using const_shared_ptr_array = std::shared_ptr<char const []>;

inline shared_ptr_array make_shared_ptr_array(std::size_t size) {
#if __cpp_lib_shared_ptr_arrays >= 201707L
    return std::make_shared<char[]>(size);
#else  // __cpp_lib_shared_ptr_arrays >= 201707L
    return std::shared_ptr<char[]>(new char[size]);
#endif // __cpp_lib_shared_ptr_arrays >= 201707L
}

} // namespace async_mqtt


#endif // defined(_DOXYGEN_)

#endif // ASYNC_MQTT_UTIL_SHARED_PTR_ARRAY_HPP
