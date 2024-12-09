// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PROTOCOL_PACKET_IMPL_PACKET_VARIANT_HPP)
#define ASYNC_MQTT_PROTOCOL_PACKET_IMPL_PACKET_VARIANT_HPP

#include <async_mqtt/protocol/packet/packet_variant.hpp>
#include <async_mqtt/protocol/packet/store_packet_variant.hpp>

namespace async_mqtt {

template <std::size_t PacketIdBytes>
template <
    typename Packet,
    std::enable_if_t<
        !std::is_same_v<
            std::decay_t<Packet>,
            basic_packet_variant<PacketIdBytes>
        > &&
        !std::is_same_v<
            std::decay_t<Packet>,
            basic_store_packet_variant<PacketIdBytes>
        >,
        std::nullptr_t
    >
>
inline
basic_packet_variant<PacketIdBytes>::basic_packet_variant(Packet&& packet):var_{std::forward<Packet>(packet)}
{}

template <std::size_t PacketIdBytes>
template <typename Func>
inline
auto basic_packet_variant<PacketIdBytes>::visit(Func&& func) const& {
    return
    std::visit(
        std::forward<Func>(func),
        var_
    );
}

template <std::size_t PacketIdBytes>
template <typename Func>
inline
auto basic_packet_variant<PacketIdBytes>::visit(Func&& func) & {
    return
    std::visit(
        std::forward<Func>(func),
        var_
    );
}

template <std::size_t PacketIdBytes>
template <typename Func>
inline
auto basic_packet_variant<PacketIdBytes>::visit(Func&& func) && {
    return
    std::visit(
        std::forward<Func>(func),
        force_move(var_)
    );
}

template <std::size_t PacketIdBytes>
template <typename T>
inline
decltype(auto) basic_packet_variant<PacketIdBytes>::get() {
    return std::get<T>(var_);
}

template <std::size_t PacketIdBytes>
template <typename T>
inline
decltype(auto) basic_packet_variant<PacketIdBytes>::get() const {
    return std::get<T>(var_);
}

template <std::size_t PacketIdBytes>
template <typename T>
inline
decltype(auto) basic_packet_variant<PacketIdBytes>::get_if() {
    return std::get_if<T>(&var_);
}

template <std::size_t PacketIdBytes>
template <typename T>
inline
decltype(auto) basic_packet_variant<PacketIdBytes>::get_if() const {
    return std::get_if<T>(&var_);
}

} // namespace async_mqtt

#endif // ASYNC_MQTT_PROTOCOL_PACKET_IMPL_PACKET_VARIANT_HPP
