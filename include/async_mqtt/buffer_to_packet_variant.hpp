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
#include <async_mqtt/packet/get_protocol_version.hpp>

namespace async_mqtt {

template <std::size_t PacketIdBytes>
basic_packet_variant<PacketIdBytes> buffer_to_basic_packet_variant(buffer buf, protocol_version ver) {
    if (buf.size() < 2) {
        return make_error(errc::bad_message, "packet size is too short");
    }
    try {
        switch (get_control_packet_type(std::uint8_t(buf[0]))) {
        case control_packet_type::connect:
            switch (ver) {
            case protocol_version::v3_1_1:
                return v3_1_1::connect_packet(force_move(buf));
            case protocol_version::v5:
                return v5::connect_packet(force_move(buf));
                break;
            case protocol_version::undetermined:
                switch (get_protocol_version(buf)) {
                case protocol_version::v3_1_1:
                    return v3_1_1::connect_packet(force_move(buf));
                case protocol_version::v5:
                    return v5::connect_packet(force_move(buf));
                default:
                    return make_error(errc::bad_message, "connect_packet protocol_version is invalid");
                }
            } break;
        case control_packet_type::connack:
            switch (ver) {
            case protocol_version::v3_1_1:
                return v3_1_1::connack_packet(force_move(buf));
            case protocol_version::v5:
                return v5::connack_packet(force_move(buf));
            default:
                return make_error(errc::bad_message, "packet mismatched to the protocol_version");
            }
            break;
        case control_packet_type::publish:
            switch (ver) {
            case protocol_version::v3_1_1:
                return v3_1_1::basic_publish_packet<PacketIdBytes>(force_move(buf));
            case protocol_version::v5:
                return v5::basic_publish_packet<PacketIdBytes>(force_move(buf));
            default:
                return make_error(errc::bad_message, "packet mismatched to the protocol_version");
            }
            break;
        case control_packet_type::puback:
            switch (ver) {
            case protocol_version::v3_1_1:
                return v3_1_1::basic_puback_packet<PacketIdBytes>(force_move(buf));
            case protocol_version::v5:
                return v5::basic_puback_packet<PacketIdBytes>(force_move(buf));
            default:
                return make_error(errc::bad_message, "packet mismatched to the protocol_version");
            }
            break;
        case control_packet_type::pubrec:
            switch (ver) {
            case protocol_version::v3_1_1:
                return v3_1_1::basic_pubrec_packet<PacketIdBytes>(force_move(buf));
            case protocol_version::v5:
                return v5::basic_pubrec_packet<PacketIdBytes>(force_move(buf));
            default:
                return make_error(errc::bad_message, "packet mismatched to the protocol_version");
            }
            break;
        case control_packet_type::pubrel:
            switch (ver) {
            case protocol_version::v3_1_1:
                return v3_1_1::basic_pubrel_packet<PacketIdBytes>(force_move(buf));
            case protocol_version::v5:
                return v5::basic_pubrel_packet<PacketIdBytes>(force_move(buf));
            default:
                return make_error(errc::bad_message, "packet mismatched to the protocol_version");
            }
            break;
        case control_packet_type::pubcomp:
            switch (ver) {
            case protocol_version::v3_1_1:
                return v3_1_1::basic_pubcomp_packet<PacketIdBytes>(force_move(buf));
            case protocol_version::v5:
                return v5::basic_pubcomp_packet<PacketIdBytes>(force_move(buf));
            default:
                return make_error(errc::bad_message, "packet mismatched to the protocol_version");
            }
            break;
        case control_packet_type::subscribe:
            switch (ver) {
            case protocol_version::v3_1_1:
                return v3_1_1::basic_subscribe_packet<PacketIdBytes>(force_move(buf));
            case protocol_version::v5:
                return v5::basic_subscribe_packet<PacketIdBytes>(force_move(buf));
            default:
                return make_error(errc::bad_message, "packet mismatched to the protocol_version");
            }
            break;
        case control_packet_type::suback:
            switch (ver) {
            case protocol_version::v3_1_1:
                return v3_1_1::basic_suback_packet<PacketIdBytes>(force_move(buf));
            case protocol_version::v5:
                return v5::basic_suback_packet<PacketIdBytes>(force_move(buf));
            default:
                return make_error(errc::bad_message, "packet mismatched to the protocol_version");
            }
            break;
        case control_packet_type::unsubscribe:
            switch (ver) {
            case protocol_version::v3_1_1:
                return v3_1_1::basic_unsubscribe_packet<PacketIdBytes>(force_move(buf));
            case protocol_version::v5:
                return v5::basic_unsubscribe_packet<PacketIdBytes>(force_move(buf));
            default:
                return make_error(errc::bad_message, "packet mismatched to the protocol_version");
            }
            break;
        case control_packet_type::unsuback:
            switch (ver) {
            case protocol_version::v3_1_1:
                return v3_1_1::basic_unsuback_packet<PacketIdBytes>(force_move(buf));
            case protocol_version::v5:
                return v5::basic_unsuback_packet<PacketIdBytes>(force_move(buf));
            default:
                return make_error(errc::bad_message, "packet mismatched to the protocol_version");
            }
            break;
        case control_packet_type::pingreq:
            switch (ver) {
            case protocol_version::v3_1_1:
                return v3_1_1::pingreq_packet(force_move(buf));
            case protocol_version::v5:
                return v5::pingreq_packet(force_move(buf));
            default:
                return make_error(errc::bad_message, "packet mismatched to the protocol_version");
            }
            break;
        case control_packet_type::pingresp:
            switch (ver) {
            case protocol_version::v3_1_1:
                return v3_1_1::pingresp_packet(force_move(buf));
            case protocol_version::v5:
                return v5::pingresp_packet(force_move(buf));
            default:
                return make_error(errc::bad_message, "packet mismatched to the protocol_version");
            }
            break;
        case control_packet_type::disconnect:
            switch (ver) {
            case protocol_version::v3_1_1:
                return v3_1_1::disconnect_packet(force_move(buf));
            case protocol_version::v5:
                return v5::disconnect_packet(force_move(buf));
            default:
                return make_error(errc::bad_message, "packet mismatched to the protocol_version");
            }
            break;
        case control_packet_type::auth:
            switch (ver) {
            case protocol_version::v5:
                return v5::auth_packet(force_move(buf));
            default:
                return make_error(errc::bad_message, "packet mismatched to the protocol_version");
            }
            break;
        default:
            break;
        }
        return make_error(errc::bad_message, "control_packet_type is invalid");
    }
    catch (system_error const& se) {
        return se;
    }
}


inline
packet_variant buffer_to_packet_variant(buffer buf, protocol_version ver) {
    return buffer_to_basic_packet_variant<2>(force_move(buf), ver);
}

} // namespace async_mqtt

#endif // ASYNC_MQTT_BUFFER_TO_PACKET_VARIANT_HPP
