// Copyright Takatoshi Kondo 2023
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PACKET_PACKET_HELPER_HPP)
#define ASYNC_MQTT_PACKET_PACKET_HELPER_HPP

#include <iosfwd>
#include <async_mqtt/packet/packet_traits.hpp>
#include <async_mqtt/packet/packet_fwd.hpp>
#include <async_mqtt/packet/packet_variant_fwd.hpp>

namespace async_mqtt {

template <typename Packet>
struct hex_dump_t {
    hex_dump_t(Packet const& p):p{p} {}

    Packet const& p;
};

template <typename Packet>
std::ostream& operator<< (std::ostream& o, hex_dump_t<Packet> const& v);

/**
 * @ingroup packet
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

#include <async_mqtt/packet/impl/packet_helper.hpp>

#endif // ASYNC_MQTT_PACKET_PACKET_HELPER_HPP
