// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PACKET_PACKET_VARIANT_HPP)
#define ASYNC_MQTT_PACKET_PACKET_VARIANT_HPP

#include <async_mqtt/variant.hpp>
#include <async_mqtt/packet/publish.hpp>

namespace async_mqtt {

template <std::size_t PacketIdBytes>
using basic_packet_variant = variant<
#if 0
    v3_1_1::connect_packet,
    v3_1_1::connack_packet,
#endif
    v3_1_1::basic_publish_packet<PacketIdBytes> // add comma later
#if 0
    v3_1_1::basic_puback_packet<PacketIdBytes>,
    v3_1_1::basic_pubrec_packet<PacketIdBytes>,
    v3_1_1::basic_pubrel_packet<PacketIdBytes>,
    v3_1_1::basic_pubcomp_packet<PacketIdBytes>,
    v3_1_1::basic_subscribe_packet<PacketIdBytes>,
    v3_1_1::basic_suback_packet<PacketIdBytes>,
    v3_1_1::basic_unsubscribe_packet<PacketIdBytes>,
    v3_1_1::basic_unsuback_packet<PacketIdBytes>,
    v3_1_1::pingreq_packet,
    v3_1_1::pingresp_packet,
    v3_1_1::disconnect_packet,
    v5::connect_packet,
    v5::connack_packet,
    v5::basic_publish_packet<PacketIdBytes>,
    v5::basic_puback_packet<PacketIdBytes>,
    v5::basic_pubrec_packet<PacketIdBytes>,
    v5::basic_pubrel_packet<PacketIdBytes>,
    v5::basic_pubcomp_packet<PacketIdBytes>,
    v5::basic_subscribe_packet<PacketIdBytes>,
    v5::basic_suback_packet<PacketIdBytes>,
    v5::basic_unsubscribe_packet<PacketIdBytes>,
    v5::basic_unsuback_packet<PacketIdBytes>,
    v5::pingreq_packet,
    v5::pingresp_packet,
    v5::disconnect_packet,
    v5::auth_packet
#endif
>;

using packet_variant = basic_packet_variant<2>;

} // namespace async_mqtt

#endif // ASYNC_MQTT_PACKET_PACKET_VARIANT_HPP
