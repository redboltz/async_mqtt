// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_UTIL_PACKET_VARIANT_OPERATOR_HPP)
#define ASYNC_MQTT_UTIL_PACKET_VARIANT_OPERATOR_HPP

#include <algorithm>

#include <async_mqtt/packet/packet_variant.hpp>
#include <async_mqtt/packet/packet_traits.hpp>

namespace async_mqtt {

namespace as = boost::asio;

template <std::size_t PacketIdBytes>
inline bool basic_packet_variant_equal(
    basic_packet_variant<PacketIdBytes> const& lhs,
    basic_packet_variant<PacketIdBytes> const& rhs
) {
    auto lhs_cbs = lhs.const_buffer_sequence();
    auto [lhs_b, lhs_e] = make_packet_range(lhs_cbs);

    auto rhs_cbs = rhs.const_buffer_sequence();
    auto [rhs_b, rhs_e] = make_packet_range(rhs_cbs);

    return std::equal(lhs_b, lhs_e, rhs_b, rhs_e);
}

inline bool packet_variant_equal(
    packet_variant const& lhs,
    packet_variant const& rhs
) {
    return basic_packet_variant_equal<2>(lhs, rhs);
}

template <std::size_t PacketIdBytes>
inline bool operator==(
    basic_packet_variant<PacketIdBytes> const& lhs,
    basic_packet_variant<PacketIdBytes> const& rhs
) {
    return basic_packet_variant_equal<PacketIdBytes>(lhs, rhs);
}

inline bool operator==(
    packet_variant const& lhs,
    packet_variant const& rhs
) {
    return packet_variant_equal(lhs, rhs);
}

template <std::size_t PacketIdBytes>
inline bool basic_packet_variant_less_than(
    basic_packet_variant<PacketIdBytes> const& lhs,
    basic_packet_variant<PacketIdBytes> const& rhs
) {
    auto lhs_cbs = lhs.const_buffer_sequence();
    auto [lhs_b, lhs_e] = make_packet_range(lhs_cbs);

    auto rhs_cbs = rhs.const_buffer_sequence();
    auto [rhs_b, rhs_e] = make_packet_range(rhs_cbs);

    return std::lexicographical_compare(lhs_b, lhs_e, rhs_b, rhs_e);
}

inline bool packet_variant_less_than(
    packet_variant const& lhs,
    packet_variant const& rhs
) {
    return basic_packet_variant_less_than<2>(lhs, rhs);
}

template <std::size_t PacketIdBytes>
inline bool operator<(
    basic_packet_variant<PacketIdBytes> const& lhs,
    basic_packet_variant<PacketIdBytes> const& rhs
) {
    return basic_packet_variant_less_than<PacketIdBytes>(lhs, rhs);
}

inline bool operator<(
    packet_variant const& lhs,
    packet_variant const& rhs
) {
    return packet_variant_less_than(lhs, rhs);
}

} // namespace async_mqtt

#endif // ASYNC_MQTT_UTIL_PACKET_VARIANT_OPERATOR_HPP
