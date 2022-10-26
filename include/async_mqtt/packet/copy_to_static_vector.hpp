// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PACKET_COPY_TO_STATIC_VECTOR_HPP)
#define ASYNC_MQTT_PACKET_COPY_TO_STATIC_VECTOR_HPP

#include <algorithm>

#include <async_mqtt/util/static_vector.hpp>
#include <async_mqtt/util/optional.hpp>
#include <async_mqtt/buffer.hpp>
#include <async_mqtt/exception.hpp>

namespace async_mqtt {

template <std::size_t N>
bool copy_advance(buffer& buf, static_vector<char, N>& sv) {
    if (buf.size() < sv.capacity()) return false;
    std::copy(
        buf.begin(),
        std::next(buf.begin(), sv.capacity()),
        std::back_inserter(sv)
    );
    buf.remove_prefix(sv.capacity());
    return true;
}

inline
optional<std::uint32_t> copy_advance_variable_length(buffer& buf, static_vector<char, 4>& sv) {
    if (buf.empty()) return nullopt;
    std::uint32_t variable_length = 0;
    auto it = buf.begin();
    // it is updated as consmed position
    if (auto len_opt = variable_bytes_to_val(it, buf.end())) {
        variable_length = *len_opt;
    }
    else {
        return nullopt;
    }
    std::copy(
        buf.begin(),
        it,
        std::back_inserter(sv));
    buf.remove_prefix(std::distance(buf.begin(), it));
    return variable_length;
}

} // namespace async_mqtt

#endif // ASYNC_MQTT_PACKET_COPY_TO_STATIC_VECTOR_HPP
