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
basic_packet_variant<PacketIdBytes> buffer_to_basic_packet_variant(buffer buf, protocol_version ver) {
    BOOST_ASSERT(buf.size() >= 2);
    switch (get_control_packet_type(buf[0])) {
    case control_packet_type::connect:
        switch (ver) {
        case protocol_version::v3_1_1:
            return v3_1_1::basic_publish_packet<PacketIdBytes>(force_move(buf));
        case protocol_version::v5:
            return v3_1_1::basic_publish_packet<PacketIdBytes>(force_move(buf));
            // return v5::basic_publish_packet<PacketIdBytes>(force_move(buf));
            break;
        case protocol_version::undetermined:
            if (buf.size() >= 7) {
                switch (static_cast<protocol_version>(buf[6])) {
                case protocol_version::v3_1_1:
                    return v3_1_1::connect_packet(force_move(buf));
                case protocol_version::v5:
                    // TDB replace to v5
                    return v3_1_1::connect_packet(force_move(buf));
                    // return v5::connect_packet(force_move(buf));
                default:
                    return make_error(errc::bad_message, "connect_packet protocol_version is invalid");
                }
            }
            else {
                return make_error(errc::bad_message, "connect_packet protocol_version doesn't exist");
            }
        } break;
    case control_packet_type::publish:
        switch (ver) {
        case protocol_version::v3_1_1:
            return v3_1_1::basic_publish_packet<PacketIdBytes>(force_move(buf));
        default:
            return make_error(errc::bad_message, "packet mismatched to the protocol_version");
        }
    default:
        return make_error(errc::bad_message, "control_packet_type is invalid");
    }
}

inline
packet_variant buffer_to_packet_variant(buffer buf, protocol_version ver) {
    return buffer_to_basic_packet_variant<2>(force_move(buf), ver);
}

} // namespace async_mqtt

#endif // ASYNC_MQTT_BUFFER_TO_PACKET_VARIANT_HPP
