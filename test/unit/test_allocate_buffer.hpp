// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_TEST_UNI_ALLOCATE_BUFFER_HPP)
#define ASYNC_MQTT_TEST_UNI_ALLOCATE_BUFFER_HPP

#include <async_mqtt/buffer.hpp>
#include <async_mqtt/util/shared_ptr_array.hpp>

namespace async_mqtt {

template <typename Iterator>
inline buffer allocate_buffer(Iterator b, Iterator e) {
    auto size = static_cast<std::size_t>(std::distance(b, e));
    if (size == 0) return buffer(&*b, size);
    auto spca = make_shared_ptr_char_array(size);
    std::copy(b, e, spca.get());
    auto p = spca.get();
    return buffer(p, size, force_move(spca));
}

inline buffer allocate_buffer(std::string_view sv) {
    return allocate_buffer(sv.begin(), sv.end());
}

} // namespace async_mqtt

#endif // ASYNC_MQTT_TEST_UNI_ALLOCATE_BUFFER_HPP
