// Copyright Takatoshi Kondo 2023
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PACKET_IMPL_PACKET_HELPER_HPP)
#define ASYNC_MQTT_PACKET_IMPL_PACKET_HELPER_HPP

#include <utility>
#include <ostream>
#include <iomanip>
#include <algorithm>
#include <async_mqtt/packet/packet_helper.hpp>
#include <async_mqtt/packet/packet_traits.hpp>
#include <async_mqtt/packet/packet_fwd.hpp>
#include <async_mqtt/packet/packet_variant_fwd.hpp>
#include <async_mqtt/packet/packet_iterator.hpp>

namespace async_mqtt {

namespace detail {

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

} // namespace detail

namespace v3_1_1 {

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

} // namespace v3_1_1

namespace v5 {

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

} // namespace v5

namespace v3_1_1 {

template <typename Lhs, typename Rhs>
std::enable_if_t<is_packet<Lhs>() && is_packet<Rhs>(), bool>
operator<(Lhs const& lhs, Rhs const& rhs) {
    auto lcbs = lhs.const_buffer_sequence();
    auto [lb, le] = make_packet_range(lcbs);
    auto rcbs = rhs.const_buffer_sequence();
    auto [rb, re] = make_packet_range(rcbs);
    return
        detail::lexicographical_compare(
            lb, le, rb, re
        );
}

} // namespace v3_1_1

namespace v5 {

template <typename Lhs, typename Rhs>
std::enable_if_t<is_packet<Lhs>() && is_packet<Rhs>(), bool>
operator<(Lhs const& lhs, Rhs const& rhs) {
    auto lcbs = lhs.const_buffer_sequence();
    auto [lb, le] = make_packet_range(lcbs);
    auto rcbs = rhs.const_buffer_sequence();
    auto [rb, re] = make_packet_range(rcbs);
    return
        detail::lexicographical_compare(
            lb, le, rb, re
        );
}

} // namespace v5

template <typename Packet>
inline std::ostream& operator<< (std::ostream& o, hex_dump_t<Packet> const& v) {
    auto cbs = v.p.const_buffer_sequence();
    auto [it, end] = make_packet_range(cbs);
    std::ios::fmtflags f(o.flags());
    o << std::hex;
    for (; it != end; ++it) {
        o << "0x" << std::setw(2) << std::setfill('0') << (static_cast<int>(*it) & 0xff) << ' ';
    }
    o.flags(f);
    return o;
}

} // namespace async_mqtt

#endif // ASYNC_MQTT_PACKET_IMPL_PACKET_HELPER_HPP
