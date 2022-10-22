// Copyright Takatoshi Kondo 2023
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PACKET_PACKET_HELPER_HPP)
#define ASYNC_MQTT_PACKET_PACKET_HELPER_HPP

#include <utility>
#include <algorithm>
#include <async_mqtt/packet/packet_iterator.hpp>
#include <async_mqtt/packet/packet_traits.hpp>
#include <async_mqtt/packet/packet_variant.hpp>

namespace async_mqtt {

template <typename Packet>
constexpr bool is_packet() {
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
        is_instance_of<v3_1_1::basic_unsuback_packet, Packet>::value ||
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
        std::is_same_v<v5::auth_packet, Packet> ||
        is_instance_of<basic_packet_variant, Packet>::value
        ;
}

template<class InputIt1, class InputIt2>
bool lexicographical_compare(
    InputIt1 first1,
    InputIt1 last1,
    InputIt2 first2,
    InputIt2 last2
) {
    for (; (first1 != last1) && (first2 != last2); ++first1, ++first2) {
        if (*first1 < *first2) return true;
        if (*first2 < *first1) return false;
    }
    return (first1 == last1) && (first2 != last2);
}

template <typename Lhs, typename Rhs>
std::enable_if_t<is_packet<Lhs>() && is_packet<Rhs>(), bool>
operator==(Lhs const& lhs, Rhs const& rhs) {
    auto lcbs = lhs.const_buffer_sequence();
    auto [lb, le] = make_packet_range(lcbs);
    auto rcbs = rhs.const_buffer_sequence();
    auto [rb, re] = make_packet_range(rcbs);
    if (std::distance(lb, le) != std::distance(rb, re)) return false;
    return std::equal(lb, le, rb);
}

template <typename Lhs, typename Rhs>
std::enable_if_t<is_packet<Lhs>() && is_packet<Rhs>(), bool>
operator<(Lhs const& lhs, Rhs const& rhs) {
    auto lcbs = lhs.const_buffer_sequence();
    auto [lb, le] = make_packet_range(lcbs);
    auto rcbs = rhs.const_buffer_sequence();
    auto [rb, re] = make_packet_range(rcbs);
    return
        lexicographical_compare(
            lb, le, rb, re
        );
}

} // namespace async_mqtt

#endif // ASYNC_MQTT_PACKET_PACKET_HELPER_HPP
