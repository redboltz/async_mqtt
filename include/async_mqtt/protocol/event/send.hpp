// Copyright Takatoshi Kondo 2024
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PROTOCOL_EVENT_SEND_HPP)
#define ASYNC_MQTT_PROTOCOL_EVENT_SEND_HPP

#include <cstddef>

#include <async_mqtt/protocol/packet/packet_variant.hpp>

namespace async_mqtt::event {

template <std::size_t PacketIdBytes>
class basic_send {
public:
    basic_send(
        basic_packet_variant<PacketIdBytes> packet,
        std::optional<typename basic_packet_id_type<PacketIdBytes>::type>
        release_packet_id_if_send_error = std::nullopt
    )
        :packet_{force_move(packet)},
         release_packet_id_if_send_error_{release_packet_id_if_send_error}
    {
    }

    basic_packet_variant<PacketIdBytes> get() const {
        return packet_;
    }
    std::optional<typename basic_packet_id_type<PacketIdBytes>::type>
    get_release_packet_id_if_send_error() const {
        return release_packet_id_if_send_error_;
    }

private:
    basic_packet_variant<PacketIdBytes> packet_;
    std::optional<typename basic_packet_id_type<PacketIdBytes>::type>
    release_packet_id_if_send_error_;
};

using send = basic_send<2>;

} // namespace async_mqtt::event

#endif // ASYNC_MQTT_PROTOCOL_EVENT_SEND_HPP
