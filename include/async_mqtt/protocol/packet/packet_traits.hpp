// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PACKET_PACKET_TRAITS_HPP)
#define ASYNC_MQTT_PACKET_PACKET_TRAITS_HPP

#include <cstdint>
#include <type_traits>

#include <async_mqtt/packet/packet_fwd.hpp>
#include <async_mqtt/packet/packet_variant_fwd.hpp>
#include <async_mqtt/packet/store_packet_variant_fwd.hpp>

namespace async_mqtt {

template <template <std::size_t> typename, typename>
struct is_instance_of : std::false_type {};

template <template <std::size_t> typename T, std::size_t N>
struct is_instance_of<T, T<N>> : std::true_type {};

template <typename Packet>
constexpr bool is_client_sendable() {
    return
        std::is_same_v<v3_1_1::connect_packet, Packet> ||
        std::is_same_v<v3_1_1::pingreq_packet, Packet> ||
        std::is_same_v<v3_1_1::disconnect_packet, Packet> ||
        is_instance_of<v3_1_1::basic_publish_packet, Packet>::value ||
        is_instance_of<v3_1_1::basic_puback_packet, Packet>::value ||
        is_instance_of<v3_1_1::basic_pubrec_packet, Packet>::value ||
        is_instance_of<v3_1_1::basic_pubrel_packet, Packet>::value ||
        is_instance_of<v3_1_1::basic_pubcomp_packet, Packet>::value ||
        is_instance_of<v3_1_1::basic_subscribe_packet, Packet>::value ||
        is_instance_of<v3_1_1::basic_unsubscribe_packet, Packet>::value ||
        std::is_same_v<v5::connect_packet, Packet> ||
        std::is_same_v<v5::pingreq_packet, Packet> ||
        std::is_same_v<v5::disconnect_packet, Packet> ||
        is_instance_of<v5::basic_publish_packet, Packet>::value ||
        is_instance_of<v5::basic_puback_packet, Packet>::value ||
        is_instance_of<v5::basic_pubrec_packet, Packet>::value ||
        is_instance_of<v5::basic_pubrel_packet, Packet>::value ||
        is_instance_of<v5::basic_pubcomp_packet, Packet>::value ||
        is_instance_of<v5::basic_subscribe_packet, Packet>::value ||
        is_instance_of<v5::basic_unsubscribe_packet, Packet>::value ||
        std::is_same_v<v5::auth_packet, Packet> ||
        is_instance_of<basic_store_packet_variant, Packet>::value
        ;
}

template <typename Packet>
constexpr bool is_server_sendable() {
    return
        std::is_same_v<v3_1_1::connack_packet, Packet> ||
        std::is_same_v<v3_1_1::pingresp_packet, Packet> ||
        is_instance_of<v3_1_1::basic_publish_packet, Packet>::value ||
        is_instance_of<v3_1_1::basic_puback_packet, Packet>::value ||
        is_instance_of<v3_1_1::basic_pubrec_packet, Packet>::value ||
        is_instance_of<v3_1_1::basic_pubrel_packet, Packet>::value ||
        is_instance_of<v3_1_1::basic_pubcomp_packet, Packet>::value ||
        is_instance_of<v3_1_1::basic_suback_packet, Packet>::value ||
        is_instance_of<v3_1_1::basic_unsuback_packet, Packet>::value ||
        std::is_same_v<v5::connack_packet, Packet> ||
        std::is_same_v<v5::pingresp_packet, Packet> ||
        std::is_same_v<v5::disconnect_packet, Packet> ||
        is_instance_of<v5::basic_publish_packet, Packet>::value ||
        is_instance_of<v5::basic_puback_packet, Packet>::value ||
        is_instance_of<v5::basic_pubrec_packet, Packet>::value ||
        is_instance_of<v5::basic_pubrel_packet, Packet>::value ||
        is_instance_of<v5::basic_pubcomp_packet, Packet>::value ||
        is_instance_of<v5::basic_suback_packet, Packet>::value ||
        is_instance_of<v5::basic_unsuback_packet, Packet>::value ||
        std::is_same_v<v5::auth_packet, Packet> ||
        is_instance_of<basic_store_packet_variant, Packet>::value
        ;
}

template <typename Packet>
constexpr bool is_v5() {
    return
        std::is_same_v<v5::connect_packet, Packet> ||
        std::is_same_v<v5::connack_packet, Packet> ||
        std::is_same_v<v5::pingreq_packet, Packet> ||
        std::is_same_v<v5::pingresp_packet, Packet> ||
        std::is_same_v<v5::disconnect_packet, Packet> ||
        is_instance_of<v5::basic_publish_packet, Packet>::value ||
        is_instance_of<v5::basic_puback_packet, Packet>::value ||
        is_instance_of<v5::basic_pubrec_packet, Packet>::value ||
        is_instance_of<v5::basic_pubrel_packet, Packet>::value ||
        is_instance_of<v5::basic_pubcomp_packet, Packet>::value ||
        is_instance_of<v5::basic_subscribe_packet, Packet>::value ||
        is_instance_of<v5::basic_suback_packet, Packet>::value ||
        is_instance_of<v5::basic_unsubscribe_packet, Packet>::value ||
        is_instance_of<v5::basic_unsuback_packet, Packet>::value ||
        std::is_same_v<v5::auth_packet, Packet>;
}

template <typename Packet>
constexpr bool is_v3_1_1() {
    return
        std::is_same_v<v3_1_1::connect_packet, Packet> ||
        std::is_same_v<v3_1_1::connack_packet, Packet> ||
        std::is_same_v<v3_1_1::pingreq_packet, Packet> ||
        std::is_same_v<v3_1_1::pingresp_packet, Packet> ||
        std::is_same_v<v3_1_1::disconnect_packet, Packet> ||
        is_instance_of<v3_1_1::basic_publish_packet, Packet>::value ||
        is_instance_of<v3_1_1::basic_puback_packet, Packet>::value ||
        is_instance_of<v3_1_1::basic_pubrec_packet, Packet>::value ||
        is_instance_of<v3_1_1::basic_pubrel_packet, Packet>::value ||
        is_instance_of<v3_1_1::basic_pubcomp_packet, Packet>::value ||
        is_instance_of<v3_1_1::basic_subscribe_packet, Packet>::value ||
        is_instance_of<v3_1_1::basic_suback_packet, Packet>::value ||
        is_instance_of<v3_1_1::basic_unsubscribe_packet, Packet>::value ||
        is_instance_of<v3_1_1::basic_unsuback_packet, Packet>::value;
}

template <typename Packet>
constexpr bool is_packet() {
    return is_v3_1_1<Packet>() || is_v5<Packet>();
}

template <typename Packet>
constexpr bool is_auth() {
    return
        std::is_same_v<v5::auth_packet, Packet>;
}

template <typename Packet>
constexpr bool is_connect() {
    return
        std::is_same_v<v3_1_1::connect_packet, Packet> ||
        std::is_same_v<v5::connect_packet, Packet>;
}

template <typename Packet>
constexpr bool is_connack() {
    return
        std::is_same_v<v3_1_1::connack_packet, Packet> ||
        std::is_same_v<v5::connack_packet, Packet>;
}

template <typename Packet>
constexpr bool is_publish() {
    return
        is_instance_of<v3_1_1::basic_publish_packet, Packet>::value ||
        is_instance_of<v5::basic_publish_packet, Packet>::value;
}

template <typename Packet>
constexpr bool is_puback() {
    return
        is_instance_of<v3_1_1::basic_puback_packet, Packet>::value ||
        is_instance_of<v5::basic_puback_packet, Packet>::value;
}

template <typename Packet>
constexpr bool is_pubrec() {
    return
        is_instance_of<v3_1_1::basic_pubrec_packet, Packet>::value ||
        is_instance_of<v5::basic_pubrec_packet, Packet>::value;
}

template <typename Packet>
constexpr bool is_pubrel() {
    return
        is_instance_of<v3_1_1::basic_pubrel_packet, Packet>::value ||
        is_instance_of<v5::basic_pubrel_packet, Packet>::value;
}

template <typename Packet>
constexpr bool is_pubcomp() {
    return
        is_instance_of<v3_1_1::basic_pubcomp_packet, Packet>::value ||
        is_instance_of<v5::basic_pubcomp_packet, Packet>::value;
}

template <typename Packet>
constexpr bool is_subscribe() {
    return
        is_instance_of<v3_1_1::basic_subscribe_packet, Packet>::value ||
        is_instance_of<v5::basic_subscribe_packet, Packet>::value;
}

template <typename Packet>
constexpr bool is_suback() {
    return
        is_instance_of<v3_1_1::basic_suback_packet, Packet>::value ||
        is_instance_of<v5::basic_suback_packet, Packet>::value;
}

template <typename Packet>
constexpr bool is_unsubscribe() {
    return
        is_instance_of<v3_1_1::basic_unsubscribe_packet, Packet>::value ||
        is_instance_of<v5::basic_unsubscribe_packet, Packet>::value;
}

template <typename Packet>
constexpr bool is_unsuback() {
    return
        is_instance_of<v3_1_1::basic_unsuback_packet, Packet>::value ||
        is_instance_of<v5::basic_unsuback_packet, Packet>::value;
}

template <typename Packet>
constexpr bool is_pingreq() {
    return
        std::is_same_v<v3_1_1::pingreq_packet, Packet> ||
        std::is_same_v<v5::pingreq_packet, Packet>;
}

template <typename Packet>
constexpr bool is_pingresp() {
    return
        std::is_same_v<v3_1_1::pingresp_packet, Packet> ||
        std::is_same_v<v5::pingresp_packet, Packet>;
}

template <typename Packet>
constexpr bool is_disconnect() {
    return
        std::is_same_v<v3_1_1::disconnect_packet, Packet> ||
        std::is_same_v<v5::disconnect_packet, Packet>;
}

template <typename Packet>
constexpr bool own_packet_id() {
    return
        is_publish<Packet>() ||
        is_pubrel<Packet>() ||
        is_subscribe<Packet>() ||
        is_unsubscribe<Packet>();
}

} // namespace async_mqtt

#endif // ASYNC_MQTT_PACKET_PACKET_TRAITS_HPP
