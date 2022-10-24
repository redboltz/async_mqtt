// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_BUFFER_TO_PACKET_VARIANT_HPP)
#define ASYNC_MQTT_BUFFER_TO_PACKET_VARIANT_HPP

#include <async_mqtt/util/optional.hpp>

#include <async_mqtt/packet/packet_variant.hpp>
#include <async_mqtt/packet/control_packet_type.hpp>
#include <async_mqtt/protocol_version.hpp>

namespace async_mqtt {

template <std::size_t PacketIdBytes>
optional<basic_packet_variant<PacketIdBytes>> buffer_to_packet_variant(buffer buf, protocol_version ver) {
    BOOST_ASSERT(buf.size() >= 2);
    switch (get_control_packet_type(buf[0])) {
    case control_packet_type::publish:
        switch (ver) {
        case protocol_version::v3_1_1:
            return v3_1_1::publish_packet(force_move(buf));
        default:
            break;
        }
    default:
        break;
    }
    return nullopt;
}

} // namespace async_mqtt

#endif // ASYNC_MQTT_BUFFER_TO_PACKET_VARIANT_HPP
