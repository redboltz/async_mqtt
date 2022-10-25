// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PACKET_COPY_TO_STATIC_VECTOR_HPP)
#define ASYNC_MQTT_PACKET_COPY_TO_STATIC_VECTOR_HPP

#include <algorithm>

#include <async_mqtt/buffer.hpp>
#include <async_mqtt/static_vector.hpp>
#include <async_mqtt/exception.hpp>

namespace async_mqtt {

void copy_advance(buffer& buf, static_vector& sv) {
    if (buf.size() < sv.capacity()) throw remaining_length_error();
    std::copy(
        buf.begin(),
        std::next(buf.begin(), sv.capacity()),
        std::back_inserter(sv)
    );
    buf.remove_prefix(sv.capacity());
}

} // namespace async_mqtt

#endif // ASYNC_MQTT_PACKET_COPY_TO_STATIC_VECTOR_HPP
