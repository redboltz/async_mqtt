// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_IMPL_BUFFER_TO_PACKET_VARIANT_IPP)
#define ASYNC_MQTT_IMPL_BUFFER_TO_PACKET_VARIANT_IPP

#include <async_mqtt/buffer_to_packet_variant.hpp>

#include <async_mqtt/packet/packet_variant.hpp>
#include <async_mqtt/packet/control_packet_type.hpp>
#include <async_mqtt/packet/v3_1_1_connect.hpp>
#include <async_mqtt/packet/v3_1_1_connack.hpp>
#include <async_mqtt/packet/v3_1_1_publish.hpp>
#include <async_mqtt/packet/v3_1_1_puback.hpp>
#include <async_mqtt/packet/v3_1_1_pubrec.hpp>
#include <async_mqtt/packet/v3_1_1_pubrel.hpp>
#include <async_mqtt/packet/v3_1_1_pubcomp.hpp>
#include <async_mqtt/packet/v3_1_1_subscribe.hpp>
#include <async_mqtt/packet/v3_1_1_suback.hpp>
#include <async_mqtt/packet/v3_1_1_unsubscribe.hpp>
#include <async_mqtt/packet/v3_1_1_unsuback.hpp>
#include <async_mqtt/packet/v3_1_1_pingreq.hpp>
#include <async_mqtt/packet/v3_1_1_pingresp.hpp>
#include <async_mqtt/packet/v3_1_1_disconnect.hpp>
#include <async_mqtt/packet/v5_connect.hpp>
#include <async_mqtt/packet/v5_connack.hpp>
#include <async_mqtt/packet/v5_publish.hpp>
#include <async_mqtt/packet/v5_puback.hpp>
#include <async_mqtt/packet/v5_pubrec.hpp>
#include <async_mqtt/packet/v5_pubrel.hpp>
#include <async_mqtt/packet/v5_pubcomp.hpp>
#include <async_mqtt/packet/v5_subscribe.hpp>
#include <async_mqtt/packet/v5_suback.hpp>
#include <async_mqtt/packet/v5_unsubscribe.hpp>
#include <async_mqtt/packet/v5_unsuback.hpp>
#include <async_mqtt/packet/v5_pingreq.hpp>
#include <async_mqtt/packet/v5_pingresp.hpp>
#include <async_mqtt/packet/v5_disconnect.hpp>
#include <async_mqtt/packet/v5_auth.hpp>
#include <async_mqtt/packet/impl/get_protocol_version.hpp>

#include <async_mqtt/util/inline.hpp>

namespace async_mqtt {

template <std::size_t PacketIdBytes>
ASYNC_MQTT_HEADER_ONLY_INLINE
std::optional<basic_packet_variant<PacketIdBytes>>
buffer_to_basic_packet_variant(
    buffer buf,
    protocol_version ver,
    error_code& ec
) {
    if (buf.size() < 2) {
        ec = make_error_code(disconnect_reason_code::malformed_packet);
        return std::nullopt;
    }
    switch (get_control_packet_type(std::uint8_t(buf[0]))) {
    case control_packet_type::connect:
        switch (ver) {
        case protocol_version::v3_1_1:
            return v3_1_1::connect_packet(force_move(buf), ec);
        case protocol_version::v5:
            return v5::connect_packet(force_move(buf), ec);
            break;
        case protocol_version::undetermined:
            switch (get_protocol_version(buf)) {
            case protocol_version::v3_1_1:
                return v3_1_1::connect_packet(force_move(buf), ec);
            case protocol_version::v5:
                return v5::connect_packet(force_move(buf), ec);
            default:
                ec = make_error_code(connect_reason_code::unsupported_protocol_version);
                return basic_packet_variant<PacketIdBytes>{};
            }
        } break;
    case control_packet_type::connack:
        switch (ver) {
        case protocol_version::v3_1_1:
            return v3_1_1::connack_packet(force_move(buf), ec);
        case protocol_version::v5:
            return v5::connack_packet(force_move(buf), ec);
        default:
            ec = make_error_code(disconnect_reason_code::protocol_error);
            return basic_packet_variant<PacketIdBytes>{};
        }
        break;
    case control_packet_type::publish:
        switch (ver) {
        case protocol_version::v3_1_1:
            return v3_1_1::basic_publish_packet<PacketIdBytes>(force_move(buf), ec);
        case protocol_version::v5:
            return v5::basic_publish_packet<PacketIdBytes>(force_move(buf), ec);
        default:
            ec = make_error_code(disconnect_reason_code::protocol_error);
            return basic_packet_variant<PacketIdBytes>{};
        }
        break;
    case control_packet_type::puback:
        switch (ver) {
        case protocol_version::v3_1_1:
            return v3_1_1::basic_puback_packet<PacketIdBytes>(force_move(buf), ec);
        case protocol_version::v5:
            return v5::basic_puback_packet<PacketIdBytes>(force_move(buf), ec);
        default:
            ec = make_error_code(disconnect_reason_code::protocol_error);
            return basic_packet_variant<PacketIdBytes>{};
        }
        break;
    case control_packet_type::pubrec:
        switch (ver) {
        case protocol_version::v3_1_1:
            return v3_1_1::basic_pubrec_packet<PacketIdBytes>(force_move(buf), ec);
        case protocol_version::v5:
            return v5::basic_pubrec_packet<PacketIdBytes>(force_move(buf), ec);
        default:
            ec = make_error_code(disconnect_reason_code::protocol_error);
            return basic_packet_variant<PacketIdBytes>{};
        }
        break;
    case control_packet_type::pubrel:
        switch (ver) {
        case protocol_version::v3_1_1:
            return v3_1_1::basic_pubrel_packet<PacketIdBytes>(force_move(buf), ec);
        case protocol_version::v5:
            return v5::basic_pubrel_packet<PacketIdBytes>(force_move(buf), ec);
        default:
            ec = make_error_code(disconnect_reason_code::protocol_error);
            return basic_packet_variant<PacketIdBytes>{};
        }
        break;
    case control_packet_type::pubcomp:
        switch (ver) {
        case protocol_version::v3_1_1:
            return v3_1_1::basic_pubcomp_packet<PacketIdBytes>(force_move(buf), ec);
        case protocol_version::v5:
            return v5::basic_pubcomp_packet<PacketIdBytes>(force_move(buf), ec);
        default:
            ec = make_error_code(disconnect_reason_code::protocol_error);
            return basic_packet_variant<PacketIdBytes>{};
        }
        break;
    case control_packet_type::subscribe:
        switch (ver) {
        case protocol_version::v3_1_1:
            return v3_1_1::basic_subscribe_packet<PacketIdBytes>(force_move(buf), ec);
        case protocol_version::v5:
            return v5::basic_subscribe_packet<PacketIdBytes>(force_move(buf), ec);
        default:
            ec = make_error_code(disconnect_reason_code::protocol_error);
            return basic_packet_variant<PacketIdBytes>{};
        }
        break;
    case control_packet_type::suback:
        switch (ver) {
        case protocol_version::v3_1_1:
            return v3_1_1::basic_suback_packet<PacketIdBytes>(force_move(buf), ec);
        case protocol_version::v5:
            return v5::basic_suback_packet<PacketIdBytes>(force_move(buf), ec);
        default:
            ec = make_error_code(disconnect_reason_code::protocol_error);
            return basic_packet_variant<PacketIdBytes>{};
        }
        break;
    case control_packet_type::unsubscribe:
        switch (ver) {
        case protocol_version::v3_1_1:
            return v3_1_1::basic_unsubscribe_packet<PacketIdBytes>(force_move(buf), ec);
        case protocol_version::v5:
            return v5::basic_unsubscribe_packet<PacketIdBytes>(force_move(buf), ec);
        default:
            ec = make_error_code(disconnect_reason_code::protocol_error);
            return basic_packet_variant<PacketIdBytes>{};
        }
        break;
    case control_packet_type::unsuback:
        switch (ver) {
        case protocol_version::v3_1_1:
            return v3_1_1::basic_unsuback_packet<PacketIdBytes>(force_move(buf), ec);
        case protocol_version::v5:
            return v5::basic_unsuback_packet<PacketIdBytes>(force_move(buf), ec);
        default:
            ec = make_error_code(disconnect_reason_code::protocol_error);
            return basic_packet_variant<PacketIdBytes>{};
        }
        break;
    case control_packet_type::pingreq:
        switch (ver) {
        case protocol_version::v3_1_1:
            return v3_1_1::pingreq_packet(force_move(buf), ec);
        case protocol_version::v5:
            return v5::pingreq_packet(force_move(buf), ec);
        default:
            ec = make_error_code(disconnect_reason_code::protocol_error);
            return basic_packet_variant<PacketIdBytes>{};
        }
        break;
    case control_packet_type::pingresp:
        switch (ver) {
        case protocol_version::v3_1_1:
            return v3_1_1::pingresp_packet(force_move(buf), ec);
        case protocol_version::v5:
            return v5::pingresp_packet(force_move(buf), ec);
        default:
            ec = make_error_code(disconnect_reason_code::protocol_error);
            return basic_packet_variant<PacketIdBytes>{};
        }
        break;
    case control_packet_type::disconnect:
        switch (ver) {
        case protocol_version::v3_1_1:
            return v3_1_1::disconnect_packet(force_move(buf), ec);
        case protocol_version::v5:
            return v5::disconnect_packet(force_move(buf), ec);
        default:
            ec = make_error_code(disconnect_reason_code::protocol_error);
            return basic_packet_variant<PacketIdBytes>{};
        }
        break;
    case control_packet_type::auth:
        switch (ver) {
        case protocol_version::v5:
            return v5::auth_packet(force_move(buf), ec);
        default:
            ec = make_error_code(disconnect_reason_code::protocol_error);
            return basic_packet_variant<PacketIdBytes>{};
        }
        break;
    default:
        break;
    }
    ec = make_error_code(disconnect_reason_code::malformed_packet);
    return std::nullopt;
}

ASYNC_MQTT_HEADER_ONLY_INLINE
std::optional<packet_variant> buffer_to_packet_variant(buffer buf, protocol_version ver, error_code& ec) {
    return buffer_to_basic_packet_variant<2>(force_move(buf), ver, ec);
}

} // namespace async_mqtt

#if defined(ASYNC_MQTT_SEPARATE_COMPILATION)

#include <async_mqtt/detail/instantiate_helper.hpp>

#define ASYNC_MQTT_INSTANTIATE_EACH(a_size) \
namespace async_mqtt { \
template \
std::optional<basic_packet_variant<a_size>> buffer_to_basic_packet_variant<a_size>( \
    buffer, \
    protocol_version, \
    error_code& \
); \
} // namespace async_mqtt

#define ASYNC_MQTT_PP_GENERATE(r, product) \
    BOOST_PP_EXPAND( \
        ASYNC_MQTT_INSTANTIATE_EACH \
        BOOST_PP_SEQ_TO_TUPLE( \
            product \
        ) \
    )

BOOST_PP_SEQ_FOR_EACH_PRODUCT(ASYNC_MQTT_PP_GENERATE, (ASYNC_MQTT_PP_SIZE))

#undef ASYNC_MQTT_PP_GENERATE
#undef ASYNC_MQTT_INSTANTIATE_EACH

#endif // defined(ASYNC_MQTT_SEPARATE_COMPILATION)

#endif // ASYNC_MQTT_IMPL_BUFFER_TO_PACKET_VARIANT_IPP
