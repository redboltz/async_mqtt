// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_UTIL_HEX_DUMP_HPP)
#define ASYNC_MQTT_UTIL_HEX_DUMP_HPP

#include <ostream>
#include <iomanip>

#include <async_mqtt/packet/packet_iterator.hpp>

namespace async_mqtt {

template <typename Packet>
struct hex_dump_t {
    hex_dump_t(Packet const& p):p{p} {}

    Packet const& p;
};

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


/**
 * @brief hexdump the packet.
 *        Usage. std::cout << hex_dump(p) << std::endl;
 * @param p packet to dump. p must be valid packet. packet_variant system_error cannot be accepted.
 * @return id
 */
template <typename Packet>
hex_dump_t<Packet> hex_dump(Packet const& p) {
    return hex_dump_t<Packet>{p};
}

} // namespace async_mqtt

#endif // ASYNC_MQTT_UTIL_HEX_DUMP_HPP)
