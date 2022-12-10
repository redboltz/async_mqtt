// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PACKET_GET_PROTOCOL_VERSION_HPP)
#define ASYNC_MQTT_PACKET_GET_PROTOCOL_VERSION_HPP

#include <async_mqtt/buffer.hpp>
#include <async_mqtt/protocol_version.hpp>
#include <async_mqtt/packet/copy_to_static_vector.hpp>

namespace async_mqtt {

inline
protocol_version get_protocol_version(buffer buf) {
    static_vector<char, 4> sv;
    // fixed_header
    buf.remove_prefix(1);
    if (auto vl_opt = insert_advance_variable_length(buf, sv)) {
        if (buf.size() >= 6) {
            return static_cast<protocol_version>(buf[6]);
        }
    }
    return protocol_version::undetermined;
}

} // async_mqtt

#endif // ASYNC_MQTT_PACKET_GET_PROTOCOL_VERSION_HPP
