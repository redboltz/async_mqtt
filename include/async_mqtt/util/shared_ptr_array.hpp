// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_UTIL_SHARED_PTR_ARRAY_HPP)
#define ASYNC_MQTT_UTIL_SHARED_PTR_ARRAY_HPP

#include <memory>

namespace async_mqtt {

/**
 * @ingroup buffer
 * @brief shared_ptr<char[]> creating function.
 * You can choose the target type.
 *   - if your compiler setting is C++20 or later, then `std::make_shared<char[]>(size)` is used.
 *      - It can allocate an array of characters and the control block in a single allocation.
 *   - otherwise `std::shared_ptr<char[]>(new char[size])` is used.
 *      - It requires two times allocations.
 * @param size of char array
 * @return shared_ptr of array
 */
inline std::shared_ptr<char []> make_shared_ptr_char_array(std::size_t size) {
#if __cpp_lib_shared_ptr_arrays >= 201707L
    return std::make_shared<char[]>(size);
#else  // __cpp_lib_shared_ptr_arrays >= 201707L
    return std::shared_ptr<char[]>(new char[size]);
#endif // __cpp_lib_shared_ptr_arrays >= 201707Lhai
}

/**
 * @ingroup buffer
 * @brief shared_ptr<char[]> creating function with allocator.
 * You can choose the target type.
 *   - if your compiler setting is C++20 or later, then `std::make_shared<char[]>(size)` is used.
 *      - It can allocate an array of characters and the control block in a single allocation.
 *   - otherwise `std::shared_ptr<char[]>(new char[size])` is used.
 *      - It requires two times allocations.
 * @param alloc allocator
 * @param size of char array
 * @return shared_ptr of array
 */
template <typename Alloc>
inline std::shared_ptr<char []> allocate_shared_ptr_char_array(Alloc&& alloc, std::size_t size) {
#if __cpp_lib_shared_ptr_arrays >= 201707L
    return std::allocate_shared<char[]>(alloc, size);
#else  // __cpp_lib_shared_ptr_arrays >= 201707L
    return std::shared_ptr<char[]>(
        alloc.allocate(size),
        [alloc, size](char* ptr) mutable { alloc.deallocate(ptr, size); }
    );
#endif // __cpp_lib_shared_ptr_arrays >= 201707Lhai
}

} // namespace async_mqtt

#endif // ASYNC_MQTT_UTIL_SHARED_PTR_ARRAY_HPP
